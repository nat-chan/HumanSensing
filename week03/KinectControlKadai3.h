#pragma once

#include <iostream>
#include <sstream>

#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>


#define _USE_MATH_defINEs
#include <math.h>

#define ERROR_CHECK(ret)		\
	if(ret != S_OK){		\
	std::stringstream ss;		\
	ss << "failed " #ret "" << std::hex << ret << std::endl;	\
	throw std::runtime_error(ss.str().c_str());		\
	}

const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;


const int PROCESS_NONE = 0;
const int PROCESS_SAVEPOSE1 = 1;
const int PROCESS_SAVEPOSE2 = 2;
const int PROCESS_SAVEPOSE3 = 3;
const int PROCESS_SAVEPOSE4 = 4;
const int PROCESS_RECOGNITION = 5;
const int MAXFRAME = 20;

class KinectControl
{
public:
	KinectControl(void);
	~KinectControl(void);

	void initialize();
	void run();

private:
	NUI_SKELETON_DATA skeleton; //DONE
	INuiSensor *kinect;
	NUI_SKELETON_DATA trackedSkeleton;

	HANDLE imageStreamHandle; //�J���[�摜�̃C�x���g
	HANDLE depthStreamHandle; //�����摜�̃C�x���g
	HANDLE streamEvent;

	DWORD width;
	DWORD height;

	void createInstance(); //�L�l�N�g�̏�����
	void setRgbImage(); //�J���[�摜�̎擾
	void setDepthImage(); //�����摜�̎擾
	void setPlayerIndex(USHORT* depth); //���[�U�[�C���f�b�N�X
	void setSkeletonImage();

	void drawTrackedSkeleton(cv::Mat& image, const NUI_SKELETON_DATA& skeleton);
	void drawBone(cv::Mat& image, const NUI_SKELETON_DATA & skeleton, NUI_SKELETON_POSITION_INDEX jointFrom, NUI_SKELETON_POSITION_INDEX jointTo);
	void drawLine(cv::Mat& image, Vector4 pos1, Vector4 pos2);
	void drawPoint(cv::Mat& image, Vector4 position);

	//�K�v������΁A�V�����֐���ϐ������܂��傤

	cv::Mat rgbIm; //�J���[�摜�s��
	cv::Mat depthIm; //�����摜�s��
	cv::Mat playerIm; //���[�U�[�C���f�b�N�X
	cv::Mat skeletonIm; //�X�P���g���f�[�^

	int processFlag;
	int savedFrameIdx;	 
};

