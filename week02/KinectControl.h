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
//const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_320x240;



class KinectControl
{
public:
	KinectControl(void);
	~KinectControl(void);

	void initialize();
	void run();

private:
	INuiSensor *kinect;
	HANDLE imageStreamHandle; //�J���[�摜�̃C�x���g
	HANDLE depthStreamHandle; //�����摜�̃C�x���g
	HANDLE streamEvent;

	DWORD width;
	DWORD height;

	void createInstance(); //�L�l�N�g�̏�����
	void setRgbImage(cv::Mat& image); //�J���[�摜�̎擾
	void setDepthImage(cv::Mat& image); //�����摜�̎擾
	void setPlayerIndex(cv::Mat& image); //���[�U�[�C���f�b�N�X

	void smoothing(cv::Mat& image);

	cv::Mat rgbIm; //�J���[�摜�s��
	cv::Mat depthIm; //�����摜�s��
	cv::Mat playerIm; //���[�U�[�C���f�b�N�X
};

