#pragma once

#include <iostream>
#include <sstream>

#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>

#define _USE_MATH_defINEs
#include <math.h>

#define ERROR_CHECK(ret)\
	if(ret != S_OK){                                              \
		std::stringstream ss;                                       \
		ss << "failed " #ret "" << std::hex << ret << std::endl;    \
		throw std::runtime_error(ss.str().c_str());                 \
	}

const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

class KinectControl{
public:
	KinectControl(void);
	~KinectControl(void);

	void initialize();
	void run();

private:
	INuiSensor *kinect;
	HANDLE imageStreamHandle; //カラー画像のイベント
	HANDLE depthStreamHandle; //距離画像のイベント
	HANDLE streamEvent;

	DWORD width;
	DWORD height;

	void createInstance();              //キネクトの初期化
	void setRgbImage(cv::Mat& image);   //カラー画像の取得
	void setDepthImage(cv::Mat& image); //距離画像の取得
	void setPlayerIndex(cv::Mat& image, USHORT* depth); //ユーザーインデックス

	void smoothing(cv::Mat& image);
	void monochrome(cv::Mat& image);
	void binarization(cv::Mat& image);
	void gaussian_filter(cv::Mat& image);
	void resetColorList();

	cv::Mat rgbIm; //カラー画像行列
	cv::Mat depthIm; //距離画像行列
	cv::Mat playerIm; //ユーザーインデックス行列

	UCHAR colorList[3][6];
	int colorflag = 0;  //ランダムカラーで r が押されたかの判定
	int BSflag = 0;  //背景を付与するかしないかの選択
};

