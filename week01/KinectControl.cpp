#include "KinectControl.h"

#define USE_RGB   1
#define USE_DEPTH 1
#define USE_NEAR  1

//カラーパレット
cv::Vec4b blue   = cv::Vec4b(255,   0,   0, 0);
cv::Vec4b green  = cv::Vec4b(  0, 255,   0, 0);
cv::Vec4b red    = cv::Vec4b(  0,   0, 255, 0);
cv::Vec4b yellow = cv::Vec4b(255, 255,   0, 0);
cv::Vec4b orange = cv::Vec4b(  0, 255, 255, 0);
cv::Vec4b purple = cv::Vec4b(255,   0, 255, 0);
cv::Vec4b black  = cv::Vec4b(  0,   0,   0, 0);

KinectControl::KinectControl(){}

KinectControl::~KinectControl(){
	//終了処理
	if(kinect != 0){
		kinect->NuiShutdown();
		kinect->Release();
	}
}

void KinectControl::initialize(){
	createInstance();

	//Kinectの初期設定
	ERROR_CHECK(kinect->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX));

	#if USE_RGB //RGB初期化
		ERROR_CHECK(kinect->NuiImageStreamOpen(
			NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION, 0, 2, 0, &imageStreamHandle));
	#endif

	#if USE_DEPTH //Depth初期化
		ERROR_CHECK(kinect->NuiImageStreamOpen(
			NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION, 0, 2, 0, &depthStreamHandle));
	#endif //CAMERA_RESOLUTIONはKinectControl.hで定義されている(640x480)

	#if USE_NEAR //Nearモードを使用するには、Nearモードのフラグをここに設定する
		ERROR_CHECK(kinect->NuiImageStreamSetImageFrameFlags(
			depthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE));
	#endif

	//フレーム更新イベントの作成
	streamEvent = ::CreateEventA(0, TRUE, FALSE, 0);
	ERROR_CHECK(kinect->NuiSetFrameEndEvent(streamEvent, 0));

	::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height);
}

void KinectControl::run(){
	LONG angle = 0;  //角度
	LONG x = 0;  //現在の角度（のちに取得）
	kinect->NuiCameraElevationSetAngle(angle);
	cv::Mat my_image = cv::Mat::zeros(height, width, CV_8UC4);
	//メインループ
	while(1){
		//更新待ち
		DWORD ret = ::WaitForSingleObject(streamEvent, INFINITE);
		::ResetEvent(streamEvent);

		//カラー画像取得と表示
		this->setRgbImage(this->rgbIm);
		cv::imshow("RGB Image", this->rgbIm);

		//距離画像取得と表示またはユーザーインデックスの表示
		this->setDepthImage(this->depthIm);
		cv::imshow("My Image", my_image);

		//キーウェイト
		int key = cv::waitKey(1);
		if(key == 'q'){
			angle = 0;
			kinect->NuiCameraElevationSetAngle(angle);
			break;
		}
		else if(key == 'a'){
			kinect->NuiCameraElevationGetAngle(&x);
			if(x > NUI_CAMERA_ELEVATION_MINIMUM && x < NUI_CAMERA_ELEVATION_MAXIMUM){
				angle = x + 1;
				kinect->NuiCameraElevationSetAngle(angle);
			}
		}
		else if(key == 'z'){
			kinect->NuiCameraElevationGetAngle(&x);
			if(x > NUI_CAMERA_ELEVATION_MINIMUM && x < NUI_CAMERA_ELEVATION_MAXIMUM){
				angle = x - 1;
				kinect->NuiCameraElevationSetAngle(angle);
			}
		}
		else if(key == 'r'){//色ローテーション
			colorflag = 1;

		}
		else if(key == 'x'){//背景とグル
			BSflag = (BSflag + 1) % 2;
		}

		if(colorflag == 1){
			resetColorList();
			colorflag = 0;
		}

	}
}

void KinectControl::resetColorList(){
	for(int s = 0; s < 3; s++){
		for(int t = 0; t < 6; t++){
			colorList[s][t] = 255 * rand();
		}
	}
}

void KinectControl::createInstance(){
	//Kinectの数を取得
	int count = 0;
	ERROR_CHECK(::NuiGetSensorCount(&count));
	if(count == 0){
		throw std::runtime_error("Kinectを接続してください");
	}

	//最初のインスタンス作成
	ERROR_CHECK(::NuiCreateSensorByIndex(0, &kinect));

	//Kinectの状態を取得
	HRESULT status = kinect->NuiStatus();
	if(status != S_OK){
		throw std::runtime_error("Kinectが利用可能ではありません");
	}
}

void KinectControl::setRgbImage(cv::Mat& image){
	// RGBカメラのフレームデータを取得する
	NUI_IMAGE_FRAME imageFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		imageStreamHandle, 0, &imageFrame));

	//画像取得
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect(0, &colorData, 0, 0);

	//画像コピー
	image = cv::Mat(height, width, CV_8UC4, colorData.pBits);

	//フレーム解放
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
		imageStreamHandle, &imageFrame));
}

void KinectControl::setDepthImage(cv::Mat& image){
	//距離画像のフレームデータを取得する
	NUI_IMAGE_FRAME depthFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		depthStreamHandle, 0, &depthFrame));


	//距離画像取得
	NUI_LOCKED_RECT depthData = {0};
	depthFrame.pFrameTexture->LockRect(0, &depthData, 0, 0);

	//
	USHORT* depth = (USHORT*)depthData.pBits;
	cv::Mat(height, width, CV_16UC1, depth).convertTo(image, CV_8UC4, 1/32.0);

	//ユーザーインデックスを取得
	this->setPlayerIndex(this->playerIm, depth);

	//フレーム解放忘れないように
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
		depthStreamHandle, &depthFrame));
}

void KinectControl::setPlayerIndex(cv::Mat& image, USHORT* depth){
	//depth: 全画素のデプスデータ（距離値+人物情報）

	//ユーザーインデックス画像の準備(image変数の初期化)(http://opencv.jp/cookbook/opencv_mat.html)
	image = cv::Mat::zeros(height, width, CV_8UC4);

	int i = 0; //depthに画素ごとのインデックス
	for(int y = 0; y < this->height; y++){
		for(int x = 0; x < this->width; x++){
			i = width*y + x;

			//NuiDepthPixelToDepthとNuiDepthPixelToPlayerIndex使うと距離データとユーザーインデックスが取得できる
			USHORT distance = NuiDepthPixelToDepth(depth[i]);
			USHORT player   = NuiDepthPixelToPlayerIndex(depth[i]);

			//距離画像座標からカラー画像座法に変換する
			LONG colorX = 0;
			LONG colorY = 0;
			kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(CAMERA_RESOLUTION, CAMERA_RESOLUTION,
				0, x, y, depth[i], &colorX, &colorY);

			if(1 <= player && player <= 6){
				image.at<cv::Vec4b>(y, x) = cv::Vec4b(colorList[0][player-1], colorList[1][player-1], colorList[2][player-1], 0);
			}else{
				if(BSflag == 0){
					image.at<cv::Vec4b>(y, x) = black;
				}else{
					image.at<cv::Vec4b>(y, x) = rgbIm.at<cv::Vec4b>(y, x); //do nothing
				}
			}
		}
	}
}

void doImageProcess(){}
