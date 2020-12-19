
#include <libCamCap.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
//#include <Windows.h>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>


class OCam
{
private:
	const int _CTRL_EXPOSURE = 5;
	const int _CTRL_GAIN = 6;
	const int _CTRL_WB_BLUE = 7;
	const int _CTRL_WB_RED = 8;
	
	long EXPOSURE = -5;
	long GAIN = 128;
	long WB_BLUE = 216;
	long  WB_RED = 121;

	int IMAGE_WIDTH = 640;
	int IMAGE_HEIGHT = 480;
	int IMAGE_FPS = 60;

	const char* mWindowName = "oCam";

	cv::Mat mRawframe;
	cv::Mat mFrame;

	int mCamNum;
	char* mCamModel;
	CAMPTR mPtrCam;

	bool mbCameraRunning = false;

public:
	enum eWIDTH_HEIGHT {W640_H480};
	enum eFPS { FPS60 };

	OCam(const eWIDTH_HEIGHT& Width_Height, const eFPS& Fps);
	~OCam();
	void SetCameraParameters(const long& exposure, const long& gain, const long& WB_Blue, const long& WB_Red);

	bool StartOCam();
	bool CloseOCam();

	cv::Mat GetFrame();

	bool IsRunningNow();
};

//��ķ ������
OCam::OCam(const eWIDTH_HEIGHT& width_Height, const eFPS& Fps)
{
	if (width_Height == W640_H480)
	{
		IMAGE_WIDTH = 640;
		IMAGE_HEIGHT = 480;
	}

	if (Fps == FPS60)
	{
		IMAGE_FPS = 60;
	}

	//ī�޶� �ѹ��� �а�, �� ���̵� ��������
	mCamNum = GetConnectedCamNumber();
	if (mCamNum == 0) printf("Can not find oCam\n");
	else std::cout << "total " << mCamNum << " devices found\n";

	mCamModel = CamGetDeviceInfo(0, INFO_MODEL_NAME);
	std::cout << "Model ID : " << mCamModel << "\n";

	//���� �����Ӱ� Raw ������ ����
	mRawframe = cv::Mat(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1); //�������ٴµ� ��ư ���� Raw ����
	mFrame = cv::Mat(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);  //�̰� RGB

}

//��ķ �Ҹ���. �������̸� �ڵ����� ��Ʈ������ �ݴ´�.
OCam::~OCam()
{
	if (mbCameraRunning == true)
	{
		CloseOCam();
	}
}

//��ķ ī�޶� �Ķ���� ����
void OCam::SetCameraParameters(const long& exposure, const long& gain, const long& WB_Blue, const long& WB_Red)
{
	EXPOSURE = exposure;
	GAIN = gain;
	WB_BLUE = WB_Blue;
	WB_RED = WB_Red;
}

//OCam ��ŸƮ ��Ű��
bool OCam::StartOCam()
{
	if (mbCameraRunning == false)
	{
		/* Open oCam */
		mPtrCam = CamOpen(0, IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_FPS, NULL, NULL);

		CamSetCtrl(mPtrCam, _CTRL_EXPOSURE, EXPOSURE);
		CamSetCtrl(mPtrCam, _CTRL_GAIN, GAIN);
		CamSetCtrl(mPtrCam, _CTRL_WB_BLUE, WB_BLUE);
		CamSetCtrl(mPtrCam, _CTRL_WB_RED, WB_RED);

		if (mPtrCam == NULL) return false;

		/* Start the camera */
		CamStart(mPtrCam);
		mbCameraRunning = true;
		return true;
	}
	else
	{
		return false;
	}
}
//OCam ��Ʈ���� ����
bool OCam::CloseOCam()
{
	if (mbCameraRunning == true)
	{
		/* Stop the streamming */
		CamStop(mPtrCam);

		/* Close the oCam */
		CamClose(mPtrCam);

		mbCameraRunning = false;
		return true;
	}
	else
	{
		return false;
	}
}

//�̹����� �������� �Լ�. �̹� ��ȯ�Ǿ� ����
cv::Mat OCam::GetFrame()
{
	if (CamGetImage(mPtrCam, mRawframe.data) == false)
	{
		std::cout << "empty frame captured!\n";
	}

	cv::cvtColor(mRawframe, mFrame, cv::COLOR_BayerGB2BGR);

	return mFrame;
}
//���� ī�޶� ���� ������ üũ�ϴ� �Լ�
bool OCam::IsRunningNow()
{
	return mbCameraRunning;
}

