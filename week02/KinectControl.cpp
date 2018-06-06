#include "KinectControl.h"


KinectControl::KinectControl()
{
}

KinectControl::~KinectControl()
{
	//終了処理
	if(kinect != 0){
		kinect->NuiShutdown();
		kinect->Release();
	}
}


void KinectControl::initialize()
{
	createInstance();

	//Kinectの初期設定
	/* 距離画像を使用できるよう記述を追加する DONE*/
	ERROR_CHECK(kinect->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX));

	//RGB初期化 DONE Depthを使うときはこちらをコメントアウトするとfpsがよくなる
//	ERROR_CHECK(kinect->NuiImageStreamOpen(
//		NUI_IMAGE_TYPE_COLOR,CAMERA_RESOLUTION,0,2,0,&imageStreamHandle));

	//Depth初期化
	/* 処理を記述 DONE*/
	ERROR_CHECK(kinect->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,CAMERA_RESOLUTION,0,2,0,&depthStreamHandle));


	//Nearモードを使用するには、Nearモードのフラグをここに設定する DONE/
	ERROR_CHECK(kinect->NuiImageStreamSetImageFrameFlags(
		depthStreamHandle,NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE));

	//フレーム更新イベントの作成
	streamEvent = ::CreateEventA(0,TRUE,FALSE,0);
	ERROR_CHECK(kinect->NuiSetFrameEndEvent(streamEvent,0));

	::NuiImageResolutionToSize(CAMERA_RESOLUTION,width,height);
}

void KinectControl::run(){
	//メインループ
	while(1){
		//更新待ち
		DWORD ret = ::WaitForSingleObject(streamEvent,INFINITE);
		::ResetEvent(streamEvent);

		//カラー画像取得と表示
//		this->setRgbImage(this->rgbIm);
//		cv::imshow("RGB Image",this->rgbIm);

		//距離画像取得と表示またはユーザーインデックスの表示
		/* 処理を記述 DONE*/
//		this->setDepthImage(this->depthIm);
//		cv::imshow("Depth Image",this->depthIm);

		this->setPlayerIndex(this->playerIm);
		this->smoothing(this->playerIm);
		cv::imshow("Player Image",this->playerIm);

		//キーウェイト
		int key = cv::waitKey(10);
		if(key == 'q'){
			break;
		}
	}
}

void KinectControl::createInstance()
{
	//Kinectの数を取得
	int count = 0;
	ERROR_CHECK(::NuiGetSensorCount(&count));
	if(count == 0){
		throw std::runtime_error("Kinectを接続してください");
	}

	//最初のインスタンス作成
	ERROR_CHECK(::NuiCreateSensorByIndex(0,&kinect));

	//Kinectの状態を取得
	HRESULT status = kinect->NuiStatus();
	if(status!=S_OK){
		throw std::runtime_error("Kinectが利用可能ではありません");
	}
}

void KinectControl::setRgbImage(cv::Mat& image)
{
	// RGBカメラのフレームデータを取得する
	NUI_IMAGE_FRAME imageFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		imageStreamHandle,0,&imageFrame));

	//画像取得
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect(0,&colorData,0,0);

	//画像コピー
	image = cv::Mat(height,width,CV_8UC4,colorData.pBits);

	//フレーム解放
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
		imageStreamHandle,&imageFrame));
}

void KinectControl::setDepthImage(cv::Mat& image)
{
	//ここに距離画像取得のための処理を書く
	//距離画像のフレームデータを取得する(setRgbImageを参考にする)
	/* 処理を記述 DONE*/
	NUI_IMAGE_FRAME depthFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		depthStreamHandle,0,&depthFrame));

	
	//距離画像取得
	NUI_LOCKED_RECT depthData = {0};
	/* 処理を記述 DONE*/
	depthFrame.pFrameTexture->LockRect(0,&depthData,0,0);


	USHORT* depth = (USHORT*)depthData.pBits;
	/* 処理を記述 DONE*/
	//13bit幅の上位8bitを取りたいので5bit右シフトする
	cv::Mat(height,width,CV_16U,depth).convertTo(image,CV_8U,1/32.0);
	


	//ユーザーインデックスを取得場合、setPlayerIndex関数がここに呼び出しましょう
	//this->setPlayerIndex(this->playerIm, depth);

	//フレーム解放忘れないように
	/* 処理を記述 DONE*/
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
			depthStreamHandle, &depthFrame));
}

void KinectControl::smoothing(cv::Mat& image){
	cv::dilate(image, image, cv::Mat(), cv::Point(-1, -1), 3);
	cv::erode(image, image, cv::Mat(), cv::Point(-1, -1), 3);
}


void KinectControl::setPlayerIndex(cv::Mat& image) {
	//depth: 全画素のデプスデータ（距離値+人物情報）

	NUI_IMAGE_FRAME depthFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		depthStreamHandle,0,&depthFrame));

	NUI_LOCKED_RECT depthData = {0};
	/* 処理を記述 DONE*/
	depthFrame.pFrameTexture->LockRect(0,&depthData,0,0);

	USHORT* depth = (USHORT*)depthData.pBits;
	
	//ユーザーインデックス画像の準備(image変数の初期化)
	//OpenCVのcv::Mat::zeros参考ください (http://opencv.jp/cookbook/opencv_mat.html)
	image = cv::Mat::zeros(height, width, CV_8U);
	/* 処理を記述 */


	int i = 0; //depthに画素ごとのインデックス
	int y = 0;
	int x = 0;
	for (y=0; y<this->height; y++) {
		for (x=0; x<this->width; x++) {
			i = width*y + x;
			//NuiDepthPixelToDepthとNuiDepthPixelToPlayerIndex使うと距離データとユーザーインデックスが取得できる

			USHORT distance = NuiDepthPixelToDepth(depth[i]);
		    USHORT player = ::NuiDepthPixelToPlayerIndex(depth[i]);

			//距離画像座標からカラー画像座法に変換する
			LONG colorX = 0;
			LONG colorY = 0;
			kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(CAMERA_RESOLUTION, CAMERA_RESOLUTION, 
				0, x, y, depth[i], &colorX, &colorY);

			if (player != 0) image.at<UCHAR>(colorY, colorX) = 100*player;
			//ここまでで点(colorX,colorY)における距離値と人物情報が得られたことになる．
			//以下省略
			//例えばif文により人物領域のみに色付けやカラー画像の画素値を割り当てたりする．
			//(このとき新たに人物表示用の行列(cv::Mat)を作成するとよい)
			
			/* 処理を記述 */
		}
	}

	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
			depthStreamHandle, &depthFrame));
}
