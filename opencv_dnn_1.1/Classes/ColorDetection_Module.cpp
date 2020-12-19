#include <vector>
#include <queue>
#include <io.h>	
#include <tuple>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>


#include "InBoxChecker_Tool.hpp"


class ColorDetection
{
private:
	struct meHueDetectionRange
	{
		int mPosZeroPoint;
		int mNegZeroPoint;
		int mMidOnePoint;

		bool mbIncludeZero;
		bool mbMidBeforeZero;

		float mAPos;
		float mXPosBias;

		float mANeg;
		float mXNegBias;

		int mSaturationThreshold;

		std::string mName;
	};

	std::vector<meHueDetectionRange> mHueDetectionRanges;
	float colorHueMapping(float mappingValue,const int& curSaturation, meHueDetectionRange* currHueDetectionRange);

	int mMedianBlurSize;
	int mDownSamplingSize;
	int mDownSamplingSize_Half;
	std::vector<uint8_t> mMedianBlurTempVector;

	std::vector<cv::Mat> mBackgroundSamples;
	std::vector<cv::Mat> mColorSamples;

	std::vector<std::string> getImageAddressFromFolder(
		const std::string& FolderAddress, 
		std::vector<std::vector<std::string>>* backgroudAddresses, 
		std::vector<std::vector<std::string>>* ColorAreaAddresses);

	std::vector<std::tuple<int, int, int>> getThresholds(
		std::vector<std::vector<std::string>>* backAddresses, 
		std::vector<std::vector<std::string>>* colorAddresses, 
		std::vector<std::tuple<int, int>>* backThresholds, 
		std::vector<std::tuple<int, int>>* colorThresholds);

public:
	enum eMedianBlurSizes { F3x3 = 3, F5x5 = 5, F7x7 = 7, F9x9 = 9, F11x11 = 11, F13x13 = 13, F15x15 = 15, F17x17 = 17, 
		F19x19 = 19, F21x21 = 21, F23x23 = 23, F25x25 = 25, F27x27 = 27, F29x29 = 29, F31x31 = 31, F33x33 = 33};
	enum eDownSamplingSizes { S3x3 = 3, S5x5 = 5, S7x7 = 7, S9x9 = 9, S11x11 = 11, S13x13 = 13, S15x15 = 15, S17x17 = 17,
		S19x19 = 19, S21x21 = 21, S23x23 = 23, S25x25 = 25, S27x27 = 27, S29x29 = 29, S31x31 = 31, S33x33 = 33};

	ColorDetection(const eMedianBlurSizes& MedianBlurSize_ = F9x9, const eDownSamplingSizes& SamplingSize_ = S11x11);
	ColorDetection(const int& positivePointZero, const int& midPointOne, const int& negativePointZero,
		const std::string& name, const eMedianBlurSizes& medianBlurSize = F9x9, const eDownSamplingSizes& samplingSize = S11x11);

	void AddColorDetectionRange(const int& positivePointZero, const int& midPointOne, const int& negativePointZero,
		const std::string& name, const int& saturationThreshold = 40);
	void ClearColorDetectionRange();
	void SetMedianBlurSize(const eMedianBlurSizes& medianBlurSize);
	void SetDownSamplingSize(const eDownSamplingSizes& samplingSize);
	int PushThroughImage(const cv::Mat& testFrame, cv::Mat* HSVedFrame,
		InBoxChecker* inBoxChecker, const int& detectionIndex = 0);

	int MakeBoxWithMedianBlur(cv::Mat* hueDetectedFrame, cv::Mat* boxFlagFrame,
		InBoxChecker* inBoxChecker, const std::string& name);
	uint8_t DoMedianBlur(cv::Mat* frame, const cv::Point& pt, const int& medianBlurHalfSize);
	int MakeBoxWithFloodFill(cv::Mat* boxFlagFrame, InBoxChecker* inBoxChecker,
		const int& originalBoxRows, const int& originalBoxCols, const std::string& name);

	void MakeColorDetection(const std::string& colorSampleFolder, const int& hueMargin = 8);
};

//������
ColorDetection::ColorDetection(const eMedianBlurSizes& medianBlurSize, const eDownSamplingSizes& samplingSize)
{
	mHueDetectionRanges.clear();
	mMedianBlurSize = medianBlurSize;
	SetDownSamplingSize(samplingSize);
	mMedianBlurTempVector = std::vector<uint8_t>(medianBlurSize * medianBlurSize);
}

ColorDetection::ColorDetection(const int& positivePointZero, const int& midPointOne, const int& negativePointZero,
	const std::string& name, const eMedianBlurSizes& medianBlurSize, const eDownSamplingSizes& samplingSize)
{
	mHueDetectionRanges.clear();
	AddColorDetectionRange(positivePointZero, midPointOne, negativePointZero, name);
	mMedianBlurSize = medianBlurSize;
	SetDownSamplingSize(samplingSize);
	mMedianBlurTempVector = std::vector<uint8_t>(medianBlurSize * medianBlurSize);
}


//���ο� �÷� ���ؼ� ������ �����.
void ColorDetection::AddColorDetectionRange(const int& positivePointZero, const int& midPointOne, const int& negativePointZero,
	const std::string& name, const int& saturationThreshold)
{
	meHueDetectionRange newRange = meHueDetectionRange();
	newRange.mPosZeroPoint = positivePointZero;
	newRange.mMidOnePoint = midPointOne;
	newRange.mNegZeroPoint = negativePointZero;
	newRange.mSaturationThreshold = saturationThreshold;

	if (positivePointZero < negativePointZero)
	{
		newRange.mbIncludeZero = true;
		newRange.mNegZeroPoint -= 255;
		if (positivePointZero < midPointOne)
		{
			newRange.mbMidBeforeZero = true;
			newRange.mMidOnePoint -= 255;
		}
	}

	newRange.mAPos = (float)(-1.0 / (newRange.mPosZeroPoint - newRange.mMidOnePoint));
	newRange.mXPosBias = (float)newRange.mPosZeroPoint;

	newRange.mANeg = (float)(1.0 / (newRange.mMidOnePoint - newRange.mNegZeroPoint));
	newRange.mXNegBias = (float)newRange.mNegZeroPoint;

	newRange.mName = name;


	mHueDetectionRanges.push_back(newRange);
}

//�÷� ���ؼ� ������ �̿��ؼ� HUE���� �����ϴ� �Լ�
float ColorDetection::colorHueMapping(float mappingValue, const int& curSaturation, meHueDetectionRange* curHueDetectionRange)
{
	meHueDetectionRange& curHueDetectionRange_ = *curHueDetectionRange;

	if (mappingValue > curHueDetectionRange_.mPosZeroPoint)
	{
		mappingValue -= 255.0;
	}

	//���� �ȿ� �ִ��� �˻�
	if ((curHueDetectionRange_.mNegZeroPoint < mappingValue) && (mappingValue < curHueDetectionRange_.mPosZeroPoint))
	{
		//�����̼� ������Ȧ�带 �Ѿ����� �˻�
		if (curSaturation >= curHueDetectionRange_.mSaturationThreshold)
		{
			//pos���� ����
			if (curHueDetectionRange_.mMidOnePoint < mappingValue)
			{
				return curHueDetectionRange_.mAPos* (mappingValue - curHueDetectionRange_.mXPosBias);

			}
			else //neg���� ����
			{
				return curHueDetectionRange_.mANeg* (mappingValue - curHueDetectionRange_.mXNegBias);
			}
		}
	}
	return 0.0;
}

//�÷� ���ؼ� �������� �ʱ�ȭ
void ColorDetection::ClearColorDetectionRange()
{
	mHueDetectionRanges.clear();
}

//�޵�� �� ���� ������ ����
void ColorDetection::SetMedianBlurSize(const eMedianBlurSizes& medianBlurSize)
{
	mMedianBlurSize = medianBlurSize;
	mMedianBlurTempVector = std::vector<uint8_t>((int)medianBlurSize * medianBlurSize);
}

void ColorDetection::SetDownSamplingSize(const eDownSamplingSizes& samplingSize)
{
	mDownSamplingSize = samplingSize;
	mDownSamplingSize_Half = samplingSize / 2;
}

//�̹����� ���μ��� �ϴ� �Լ�
int ColorDetection::PushThroughImage(const cv::Mat& testFrame, cv::Mat* HSVedFrame, 
	InBoxChecker* inBoxChecker, const int& detectionIndex)
{
	auto& HSVedFrame_ = *HSVedFrame;
	auto& inBoxChecker_ = *inBoxChecker;

	cv::Mat HSV_Frame = cv::Mat(testFrame.rows, testFrame.cols, CV_8UC3);
	cv::Mat detectedFrame = cv::Mat::zeros(testFrame.rows, testFrame.cols, CV_8UC1);
	//cv::Mat medianBluredFrame;
	cv::cvtColor(testFrame, HSV_Frame, cv::COLOR_BGR2HSV);
	cv::Vec3b* HSVframePtr;
	uchar* detectedFramePtr = detectedFrame.ptr(0);

	float hueVal;
	//int jumping_width;

	int flagRow = (testFrame.rows - mDownSamplingSize_Half - 1) / mDownSamplingSize + 1;
	int flagCol = (testFrame.cols - mDownSamplingSize_Half - 1) / mDownSamplingSize + 1;
	cv::Mat BoxFlagFrame = cv::Mat::zeros(flagRow + 2, flagCol + 2, CV_8UC1);

	//Hue�÷��� ������ ����� ����
	for (int r = 0; r < HSV_Frame.rows; r++)
	{
		HSVframePtr = HSV_Frame.ptr<cv::Vec3b>(r);
		detectedFramePtr = detectedFrame.ptr(r);
		for (int c = 0; c < HSV_Frame.cols; c++)
		{

			hueVal = colorHueMapping(HSVframePtr[c][0], HSVframePtr[c][1], &mHueDetectionRanges[detectionIndex]);
			//127������ �ϴ� ������ �� �պ�Ʈ�� Flag�� ���� ���ؼ��̴�.
			detectedFramePtr[c] = (int)(hueVal * 127);
		}
	}


	//�޵�� ���� ���� �ٿ���ø� ���� �̾Ƴ��� ���� �����Ѵ�.
	//��ȯ���� �÷��׸ʿ��� ������ �� �÷����� �ȼ��� ����*�ٿ���ø� ũ��^2 ���� ���ȴ�.
	int detectedPixelCount = MakeBoxWithMedianBlur(&detectedFrame, &BoxFlagFrame, &inBoxChecker_ ,mHueDetectionRanges[detectionIndex].mName);
	cv::Mat testTempFrame;
	cv::resize(BoxFlagFrame, testTempFrame, cv::Size(detectedFrame.cols, detectedFrame.rows), 0, 0, cv::InterpolationFlags::INTER_LINEAR);
	cv::imshow("sampling", testTempFrame);
	cv::imwrite("./imwrite_imgs/sampling.jpg", testTempFrame);

	cv::imshow("HSV_Hue mapped frame", detectedFrame);
	cv::imwrite("./imwrite_imgs/HSV_Hue mapped frame.jpg", detectedFrame);

	cv::Mat medianBluredFrame;
	cv::medianBlur(detectedFrame, medianBluredFrame, mMedianBlurSize);
	cv::imshow("sampling_medianBlured", medianBluredFrame);
	cv::imwrite("./imwrite_imgs/sampling_medianBlured.jpg", medianBluredFrame);


	HSV_Frame.copyTo(HSVedFrame_);
	return detectedPixelCount;
};

//�޵�� ���� ����ؼ� �ڽ��� �����. ���������δ� FloodFill �˰��� ���Ǿ���.
int ColorDetection::MakeBoxWithMedianBlur(cv::Mat* hueDetectedFrame, cv::Mat* boxFlagFrame, 
	InBoxChecker* inBoxChecker,const std::string& name)
{
	auto& hueDetectedFrame_ = *hueDetectedFrame;
	auto& boxFlagFrame_ = *boxFlagFrame;
	auto& inBoxChecker_ = *inBoxChecker;


	int jumpingWidth = 0;
	int medianBlurHalf = mMedianBlurSize / 2;

	uchar* boxFlagFramePtr;
	uchar* detectedFramePtr;

	int flagRow = 1;
	int flagCol = 1;

	for (int r = mDownSamplingSize_Half; r < hueDetectedFrame_.rows; r += mDownSamplingSize)
	{
		detectedFramePtr = hueDetectedFrame_.ptr(r);
		boxFlagFramePtr = boxFlagFrame_.ptr(flagRow);

		flagCol = 1;
		for (int c = mDownSamplingSize_Half; c < hueDetectedFrame_.cols; c += mDownSamplingSize)
		{
			if (inBoxChecker_.CheckInBox(cv::Point(c, r), &jumpingWidth))
			{
				c = c + ((((jumpingWidth) / mDownSamplingSize) + 1) * mDownSamplingSize);
				flagCol = ((c - 4) / mDownSamplingSize) + 1;
				flagCol++;
				continue;
			}
			boxFlagFramePtr[flagCol] = DoMedianBlur(&hueDetectedFrame_, cv::Point(c, r), medianBlurHalf);
			auto asdf = boxFlagFramePtr[flagCol];

			flagCol++;
		}
		flagRow++;
	}

	int totalDetectedPixelCount = MakeBoxWithFloodFill(&boxFlagFrame_, &inBoxChecker_, hueDetectedFrame_.rows, hueDetectedFrame_.cols,name);

	return totalDetectedPixelCount;
};

//�ѹ� ���� �� �� �ڽ��÷��׸�Ʈ������ �̿��ؼ� �ڽ��� ���� �ιڽ�üĿ�� ����ϴ� �Լ�
//��ȯ���� ������ �� �ȼ� ���̴�.
int ColorDetection::MakeBoxWithFloodFill(cv::Mat* boxFlagFrame, InBoxChecker* inBoxChecker, 
	const int& originalBoxRows, const int& originalBoxCols, const std::string& name)
{
	auto& boxFlagFrame_ = *boxFlagFrame;
	auto& inBoxChecker_ = *inBoxChecker;

	uchar* boxFlagFramePtr;

	std::queue<cv::Point> pointQue;
	uchar val;
	int addWidth;

	cv::Point curPt;

	cv::Point pt_Up;
	cv::Point pt_Down;
	cv::Point pt_Left;
	cv::Point pt_Right;

	cv::Point pt_UpLeft;
	cv::Point pt_DownRight;

	int totalDetectedPixel = 0;

	for (int r = 1; r < boxFlagFrame_.rows - 1; r++)
	{
		boxFlagFramePtr = boxFlagFrame_.ptr(r);
		for (int c = 1; c < boxFlagFrame_.cols - 1; c++)
		{
			//�̹� �ִ� �ڽ��� ���Եȴٸ� ������ �ѱ��
			if (inBoxChecker_.CheckInBox(cv::Point((c - 1) * mDownSamplingSize + mDownSamplingSize_Half,
				(r - 1) * mDownSamplingSize + mDownSamplingSize_Half), &addWidth))
			{
				c = c + ((addWidth / mDownSamplingSize) + 1);
				continue;
			}

			val = boxFlagFramePtr[c];

			//floodfill ����
			if (val > 0)
			{
				//���� �Ⱥ�� �ִٸ� �� ���� ������ ������ ������ ���̹Ƿ�.. ���� ���Ӱ� �ʱ�ȭ�� �ʿ�� ���ڴ�.
				pointQue.push(cv::Point(c, r));

				pt_UpLeft = cv::Point(c, r);
				pt_DownRight = cv::Point(c, r);

				boxFlagFrame_.at<uchar>(cv::Point(c, r)) += 0x80;

				while (pointQue.empty() == false)
				{
					//�ϴ� �ȼ��� �����Ǿ����� �ϳ� �߰��ϰ� ����
					totalDetectedPixel++;

					curPt = pointQue.front();
					pointQue.pop();

					pt_Up = curPt;
					pt_Up.y -= 1;

					pt_Down = curPt;
					pt_Down.y += 1;

					pt_Left = curPt;
					pt_Left.x -= 1;

					pt_Right = curPt;
					pt_Right.x += 1;

					//������ �˻�,�� ���� ��Ʈ�� 128�� �ǹ��ϴ� ��Ʈ�� �����ϴ��� �˻��Ѵ�. �̰��� ��ü flag�� �����Ѵ�.
					if (boxFlagFrame_.at<uchar>(pt_Right) > 0)
					{
						if ((boxFlagFrame_.at<uchar>(pt_Right) >> 7) == 0x00)
						{
							//�˻簡 �ȵǾ��־��ٸ�, ť�� ���Ӱ� �ְ�,
							pointQue.push(pt_Right);
							if (pt_Right.x > pt_DownRight.x) pt_DownRight.x = pt_Right.x;
							boxFlagFrame_.at<uchar>(pt_Right) += 0x80;
						}
					}
					//�Ʒ��� �˻�
					if (boxFlagFrame_.at<uchar>(pt_Down) > 0)
					{
						if ((boxFlagFrame_.at<uchar>(pt_Down) >> 7) == 0x00)
						{
							pointQue.push(pt_Down);
							if (pt_Down.y > pt_DownRight.y) pt_DownRight.y = pt_Down.y;
							boxFlagFrame_.at<uchar>(pt_Down) += 0x80;
						}
					}
					//���� �˻�
					if (boxFlagFrame_.at<uchar>(pt_Left) > 0)
					{
						if ((boxFlagFrame_.at<uchar>(pt_Left) >> 7) == 0x00)
						{
							pointQue.push(pt_Left);
							if (pt_Left.x < pt_UpLeft.x) pt_UpLeft.x = pt_Left.x;
							boxFlagFrame_.at<uchar>(pt_Left) += 0x80;
						}
					}
					//���� �˻�
					if (boxFlagFrame_.at<uchar>(pt_Up) > 0)
					{
						if ((boxFlagFrame_.at<uchar>(pt_Up) >> 7) == 0x00)
						{
							pointQue.push(pt_Up);
							if (pt_Up.y < pt_UpLeft.y) pt_UpLeft.y = pt_Up.y;
							boxFlagFrame_.at<uchar>(pt_Up) += 0x80;
						}
					}
				}

				//�ڽ����� ���� ũ��+�ѿ� ��ĭ�� �� �鸮��
				pt_UpLeft = (pt_UpLeft - cv::Point(2, 2)) * mDownSamplingSize + cv::Point(mDownSamplingSize_Half, mDownSamplingSize_Half);
				pt_DownRight = (pt_DownRight * mDownSamplingSize + cv::Point(mDownSamplingSize_Half, mDownSamplingSize_Half));

				//���� ������ ������ ����� �ʾҴ��� �˻�
				if (pt_UpLeft.x < 0) pt_UpLeft.x = 0;
				if (pt_UpLeft.y < 0) pt_UpLeft.y = 0;
				if (pt_DownRight.x > originalBoxCols - 1) pt_DownRight.x = originalBoxCols - 1;
				if (pt_DownRight.y > originalBoxRows - 1) pt_DownRight.y = originalBoxRows - 1;



				cv::Rect newRect = cv::Rect(pt_UpLeft, pt_DownRight);
				inBoxChecker_.AddBox(newRect, InBoxChecker::ColorDetection, name);


			}

		}
	}

	//�ѹ��� ������ �ȼ��� ������ ���� ������� �� ����� ����.
	//���� �÷��׸��� �ٿ���ø� ����� ���̿��Ͽ�, �÷��׸��� 1ĭ�� �ٿ���ø�������^2���� �ȼ��� �����Ѱ����� �����Ѵ�.
	//�ӵ��� ������������, ��� ��Ȯ���� �پ��� Ʈ���̵� ������ �����.
	totalDetectedPixel = totalDetectedPixel * mDownSamplingSize * mDownSamplingSize;

	return totalDetectedPixel;
};

//Ư�� ������ �޵�� ���� �����ϴ� �Լ�. ���̳ʽ� ��ǥ�� ���ؼ��� ���̾���.
uint8_t ColorDetection::DoMedianBlur(cv::Mat* frame, const cv::Point& pt, const int& medianBlurHalfSize)
{
	auto& frame_ = *frame;

	uchar* framePtr;;// = frame.ptr<uchar>(0);
	uint8_t i = 0;
	uint8_t correct = 0;

	for (int r = pt.y - medianBlurHalfSize; r <= pt.y + medianBlurHalfSize; r++)
	{

		for (int c = pt.x - medianBlurHalfSize; c <= pt.x + medianBlurHalfSize; c++)
		{
			if ((r >= frame_.rows) || (c >= frame_.cols) || (r < 0) || (c < 0))
			{
				mMedianBlurTempVector[i] = 0xFF;
				i++;
				continue;
			}
			framePtr = frame_.ptr<uchar>(r);
			mMedianBlurTempVector[i] = framePtr[c];
			i++;
			correct++;
		}
	}

	std::sort(mMedianBlurTempVector.begin(), mMedianBlurTempVector.end());

	return mMedianBlurTempVector[(correct + 1) / 2 - 1];

};


//�÷� ���ؼ� �Ķ���͸� �ڵ����� �������ִ� �Լ�
//���� ����� �̸�, ���ο� ���ϴ� Ŭ���� ���ڸ�ŭ ���� ����,�� �ȿ��� �ٽ� Background�� ColorArea���� �����س�����
//�̹��� ������ .png .bmp. jpg�� �о����
void ColorDetection::MakeColorDetection(const std::string& colorSampleFolder, const int& hueMargin)
{
	std::vector<std::vector<std::string>> backgroundImageAddresses;
	std::vector < std::vector<std::string>> colorSampleImageAddresses;
	std::vector<std::string> classes;

	classes = getImageAddressFromFolder(colorSampleFolder, &backgroundImageAddresses, &colorSampleImageAddresses);

	//<�� ���� sat, 95%�� ���� sat
	std::vector<std::tuple<int, int>> backgroundSatThresholds;
	//�� ���� sat, 95%�� ���� sat
	std::vector<std::tuple<int, int>> colorAreaSatThresholds;
	//positive, mid ,negative hue ������
	std::vector<std::tuple<int, int, int>> colorAreaHueMappings;

	colorAreaHueMappings = getThresholds(&backgroundImageAddresses, &colorSampleImageAddresses, &backgroundSatThresholds, &colorAreaSatThresholds);

	bool isHueAlreadyExist = false;

	for (int i = 0; i < classes.size(); i++)
	{
		isHueAlreadyExist = false;
		for (int ii = 0; ii < mHueDetectionRanges.size(); ii++)
		{
			if (mHueDetectionRanges[ii].mName == classes[i])
			{
				isHueAlreadyExist = true;
				break;
			}
		}

		if (isHueAlreadyExist) continue;

		uint8_t satThreshold = 0;
		uint8_t back_top = std::get<0>(backgroundSatThresholds[i]);
		uint8_t back_bot_confidence = std::get<1>(backgroundSatThresholds[i]);
		uint8_t color_bot = std::get<0>(colorAreaSatThresholds[i]);
		uint8_t color_top_confidence = std::get<1>(colorAreaSatThresholds[i]);

		if (back_top <= color_bot)
		{
			satThreshold = color_bot;
		}
		else if (color_top_confidence <= back_bot_confidence)
		{
			satThreshold = color_top_confidence;
		}
		else if (back_bot_confidence <= color_bot)
		{
			if (back_top <= color_top_confidence)
			{
				satThreshold = back_top;
			}
			else
			{
				satThreshold = color_top_confidence;
			}
		}
		else if (color_bot < back_bot_confidence)
		{
			satThreshold = back_bot_confidence;
		}


		uint8_t hue_pos = std::get<0>(colorAreaHueMappings[i]) + hueMargin;
		if (hue_pos > 0xFF) hue_pos -= 0xFF;
		uint8_t hue_neg = std::get<2>(colorAreaHueMappings[i]) - hueMargin;
		if (hue_neg < 0) hue_neg += 0xFF;


		AddColorDetectionRange(hue_pos, std::get<1>(colorAreaHueMappings[i]),
			hue_neg, classes[i], satThreshold);
	}
}

//��׶��� �̹������� ��ε�, �÷����� �̹������� ��ε��� ref�� ��������, classes�� ��ȯ�ϴ� �Լ�
std::vector<std::string> ColorDetection::getImageAddressFromFolder(const std::string& folderAddress, 
	std::vector<std::vector<std::string>>* backgroudAddresses, 
	std::vector<std::vector<std::string>>* colorAreaAddresses)
{
	auto& backgroudAddresses_ = *backgroudAddresses;
	auto& colorAreaAddresses_ = *colorAreaAddresses;

	std::string searching = ".\\" + "autoColorParameters" +"\\" + folderAddress + "\\" + "*.*";
	std::vector<std::string> colorFolders;
	std::vector<std::string> classes;

	_finddata_t fd;
	intptr_t handle;
	int result;
	handle = _findfirst(searching.c_str(), &fd);

	std::string fileName;
	const char* tempChar;

	//���ο� ���ϴ� Ŭ���� ���� Ž��
	if (handle == -1)
	{
		printf("there is no File!\n");
	}

	while (true)
	{
		fileName = fd.name;
		std::cout << fileName << std::endl;
		tempChar = strrchr(fileName.c_str(), '.');

		if (tempChar == NULL)
		{
			handle = _findfirst((".\\" + folderAddress + "\\" + fileName + "*.*").c_str(), &fd);
			if (handle == -1)
			{
				std::cout << "this is not folder! : " << fileName << std::endl;
			}
			else
			{
				colorFolders.push_back(".\\" + folderAddress + "\\" + fileName);
				classes.push_back(fileName);
			}
		}

		result = _findnext(handle, &fd);
		if (result != 0)
		{
			break;
		}

	}



	std::string fileExtension;

	//Background ������ �����Ͽ� �̹������� �ּҵ� ������
	for (int i = 0; i < colorFolders.size(); i++)
	{
		handle = _findfirst((colorFolders[i] + "\\Background\\*.*").c_str(), &fd);
		std::vector<std::string> newImages;

		if (handle == -1)
		{
			printf("there is no File!\n");
		}

		while (true)
		{
			fileName = fd.name;
			tempChar = strrchr(fileName.c_str(), '.');
			if (tempChar == NULL)
			{
				result = _findnext(handle, &fd);
				if (result != 0)
				{
					break;
				}
				continue;
			}

			fileExtension = tempChar;

			for (int i = 0; i < fileExtension.size(); i++)
			{
				fileExtension[i] = tolower(fileExtension[i]);
			}

			if ((fileExtension == ".jpg") || (fileExtension == ".bmp") || (fileExtension == ".png"))
			{
				newImages.push_back(colorFolders[i] + "\\Background\\" + fd.name);
			}

			result = _findnext(handle, &fd);
			if (result != 0)
			{
				break;
			}
		}

		backgroudAddresses_.push_back(newImages);
	}

	//ColorArea ������ �����Ͽ� �̹������� �ּҵ� ������
	for (int i = 0; i < colorFolders.size(); i++)
	{
		handle = _findfirst((colorFolders[i] + "\\ColorArea\\*.*").c_str(), &fd);
		std::vector<std::string> newImages;

		if (handle == -1)
		{
			printf("there is no File!\n");
		}

		while (true)
		{
			fileName = fd.name;
			tempChar = strrchr(fileName.c_str(), '.');
			if (tempChar == NULL) continue;

			fileExtension = tempChar;

			if (tempChar == NULL)
			{
				result = _findnext(handle, &fd);
				if (result != 0)
				{
					break;
				}
				continue;
			}

			for (int i = 0; i < fileExtension.size(); i++)
			{
				fileExtension[i] = tolower(fileExtension[i]);
			}

			if ((fileExtension == ".jpg") || (fileExtension == ".bmp") || (fileExtension == ".png"))
			{
				newImages.push_back(colorFolders[i] + "\\ColorArea\\" + fd.name);
			}

			result = _findnext(handle, &fd);
			if (result != 0)
			{
				break;
			}
		}

		colorAreaAddresses_.push_back(newImages);
	}

	_findclose(handle);

	return classes;
}

//�����̼� 95%�� 100%�� ��ȯ�ϴ� �Լ�
std::vector<std::tuple<int, int, int>> ColorDetection::getThresholds(
	std::vector<std::vector<std::string>>* backAddresses, 
	std::vector<std::vector<std::string>>* colorAddresses, 
	std::vector<std::tuple<int, int>>* backThresholds, 
	std::vector<std::tuple<int, int>>* colorThresholds)
{
	auto& backAddresses_ = *backAddresses;
	auto& colorAddresses_ = *colorAddresses;
	auto& backThresholds_ = *backThresholds;
	auto& colorThresholds_ = *colorThresholds;


	cv::Mat frame;
	cv::Mat HSVFrame;
	cv::Vec3b* medianFilteredHSVFramePtr;
	cv::Mat MedianFilteredHSVFrame;
	std::priority_queue<uint8_t, std::vector<uint8_t>, std::less<uint8_t>> backPQ;
	std::priority_queue<uint8_t, std::vector<uint8_t>, std::greater<uint8_t>> colorPQ;
	std::vector<uint8_t> hueValues;

	std::vector<std::tuple<int, int, int>> hueMappings;

	int totalPixel = 0;

	//������ �����̼� ������� ���
	for (int i = 0; i < backAddresses_.size(); i++)
	{
		std::tuple<int, int> temp = { 0,0 };

		auto tempAddresses = backAddresses_[i];


		for (int ii = 0; ii < tempAddresses.size(); ii++)
		{
			frame = cv::imread(tempAddresses[ii]);
			if (frame.empty() != true)
			{
				std::cout << "processing... : " << tempAddresses[ii] << std::endl;

				cv::cvtColor(frame, HSVFrame, cv::COLOR_BGR2HSV);
				cv::medianBlur(HSVFrame, MedianFilteredHSVFrame, mMedianBlurSize);

				for (int r = 0; r < MedianFilteredHSVFrame.rows; r++)
				{
					for (int c = 0; c < MedianFilteredHSVFrame.cols; c++)
					{
						medianFilteredHSVFramePtr = MedianFilteredHSVFrame.ptr<cv::Vec3b>(r);

						backPQ.push(medianFilteredHSVFramePtr[c][1]);
					}
				}
				totalPixel = totalPixel + MedianFilteredHSVFrame.cols * MedianFilteredHSVFrame.rows;
			}
			else
			{
				std::cout << "processing failed : " << tempAddresses[ii] << std::endl;
			}
		}

		uint8_t top = backPQ.top();

		int sat95 = static_cast<int>(totalPixel * 0.05f);

		for (int sat = 0; sat < sat95; sat++)
		{
			backPQ.pop();
		}

		uint8_t top95 = backPQ.top();

		temp = std::make_tuple(top, top95);

		backThresholds_.push_back(temp);

		while (!backPQ.empty()) backPQ.pop();
	}

	totalPixel = 0;

	//�÷� �����̼� ������� + �� �÷����� ���
	for (int i = 0; i < colorAddresses_.size(); i++)
	{
		std::tuple<int, int> temp_int_int = { 0,0 };
		auto tempAddresses = colorAddresses_[i];
		hueValues.clear();

		for (int ii = 0; ii < tempAddresses.size(); ii++)
		{
			frame = cv::imread(tempAddresses[ii]);
			if (frame.empty() != true)
			{
				std::cout << "processing... : " << tempAddresses[ii] << std::endl;

				cv::cvtColor(frame, HSVFrame, cv::COLOR_BGR2HSV);
				cv::medianBlur(HSVFrame, MedianFilteredHSVFrame, mMedianBlurSize);

				for (int r = 0; r < MedianFilteredHSVFrame.rows; r++)
				{
					for (int c = 0; c < MedianFilteredHSVFrame.cols; c++)
					{
						medianFilteredHSVFramePtr = MedianFilteredHSVFrame.ptr<cv::Vec3b>(r);
						auto h_ = medianFilteredHSVFramePtr[c][0];
						auto s_ = medianFilteredHSVFramePtr[c][1];
						auto v_ = medianFilteredHSVFramePtr[c][2];
						colorPQ.push(medianFilteredHSVFramePtr[c][1]);
						hueValues.push_back(medianFilteredHSVFramePtr[c][0]);
						//Hue �÷� �����Ϸ��� �����̾Ƽ ť�� ����
					}
				}
				totalPixel = totalPixel + HSVFrame.cols * HSVFrame.rows;

			}
			else
			{
				std::cout << "processing failed : " << tempAddresses[ii] << std::endl;
			}
		}

		uint8_t top = colorPQ.top();

		int sat95 = (float)totalPixel * 0.05f;

		for (int sat = 0; sat < sat95; sat++)
		{
			colorPQ.pop();
		}

		uint8_t top95 = colorPQ.top();
		temp_int_int = std::make_tuple(top, top95);
		colorThresholds_.push_back(temp_int_int);


		float hueConfidencePercentage = 0.05f;
		int hueConfidenceCount = static_cast<int>(hueValues.size() * hueConfidencePercentage);
		std::sort(hueValues.begin(), hueValues.end());

		uint8_t huebotomWithConfidence = hueValues[hueConfidenceCount];
		uint8_t hueTopWithConfidence = hueValues[hueValues.size() - hueConfidenceCount];
		bool isZeroInclude = false;

		//ž�� ������ ���� 127 �̻� ���̳����, ���̿� 0�� ����ִ°����� �Ǵ���
		//���� ž�� ���Ұ��� �̸� �ٲ����
		if (hueTopWithConfidence - huebotomWithConfidence > 0x7F)
		{
			isZeroInclude = true;
			uint8_t tempSwap = hueTopWithConfidence;
			hueTopWithConfidence = huebotomWithConfidence;
			huebotomWithConfidence = tempSwap;
		}

		double hueMid = 0;
		int count = 1;

		for (int i = sat95; i < totalPixel - sat95; i++)
		{
			//0�� ������ �����ϰ��, 127�� �Ѵ� ���鿡�� -255�� ���ؼ� ���̳ʽ��� ����� ����� ����Ѵ�
			if (isZeroInclude && (hueValues[i] > 0x7F))
			{
				hueMid = (hueMid * (count - 1) / (double)count) + (((double)hueValues[i] - 255.0) / count);
			}
			else
			{
				hueMid = (hueMid * (count - 1) / (double)count) + ((double)hueValues[i] / count);
			}
			count++;
		}

		//0�� ������ �����������, mid�� ���� -��, ������ ���� +255�� �����ش�
		if (isZeroInclude && (hueMid <= 0.0))
		{
			hueMid += 255;
		}
		std::tuple<int, int, int> temp_int3;
		temp_int3 = std::make_tuple(hueTopWithConfidence, hueMid, huebotomWithConfidence);
		hueMappings.push_back(temp_int3);


		while (!colorPQ.empty()) colorPQ.pop();
	}


	return hueMappings;

}
