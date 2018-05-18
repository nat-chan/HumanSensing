#include "KinectControlKadai3.h"


KinectControl::KinectControl()
{
	skeleton.eTrackingState = NUI_SKELETON_NOT_TRACKED; //DONE
	processFlag = PROCESS_NONE;
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
	ERROR_CHECK(kinect->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON));

	//RGB初期化
	ERROR_CHECK(kinect->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,CAMERA_RESOLUTION,0,2,0,&imageStreamHandle));

	//Depth初期化
	ERROR_CHECK(kinect->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION, 0, 2, 0, &depthStreamHandle));

	//Nearモードを使用するには、Nearモードのフラグをここに設定する
	ERROR_CHECK(kinect->NuiImageStreamSetImageFrameFlags(depthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE));

	//skeleton初期化
	ERROR_CHECK(kinect->NuiSkeletonTrackingEnable(0,NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE | NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT));
		

	//フレーム更新イベントの作成
	streamEvent = ::CreateEventA(0,TRUE,FALSE,0);
	ERROR_CHECK(kinect->NuiSetFrameEndEvent(streamEvent,0));

	::NuiImageResolutionToSize(CAMERA_RESOLUTION,width,height);
}

void KinectControl::run()
{
	//メインループ
	while(1){
		//更新待ち
		DWORD ret = ::WaitForSingleObject(streamEvent,INFINITE);
		::ResetEvent(streamEvent);

		//カラー画像取得と表示
		setRgbImage();
		setDepthImage();
		setSkeletonImage();

		cv::imshow("RGB Image",rgbIm);
		cv::imshow("Depth",depthIm);
		cv::imshow("Player",playerIm);
		cv::imshow("Skeleton",skeletonIm);

		//キーウェイト
		int key = cv::waitKey(10);
		if(key == 'q') {
			break;
		}
		switch(key) {
		case 'x':
			//複数データを保存する前に、一枚データを保存してみましょう
			//うまくできたら、case '1', '2', ...　に進んでください
			break;
		case '1':
			std::cout << "ポーズ1の辞書データの取得を開始する" << std::endl;
			//フラグを設定する、保存されたフレーム数(savedFrameIdx)を初期化する
			//...
			break;
		case '2':
			std::cout << "ポーズ2の辞書データの取得を開始する" << std::endl;
			//フラグを設定する、保存されたフレーム数を初期化する
			//...
			break;
		case '3':
			std::cout << "ポーズ3の辞書データの取得を開始する" << std::endl;
			//フラグを設定する、保存されたフレーム数を初期化する
			//...
			break;
		case '4':
			std::cout << "ポーズ4の辞書データの取得を開始する" << std::endl;
			//フラグを設定する、保存されたフレーム数を初期化する
			//...
			break;
		case 'a':
			std::cout << "認識を開始する" << std::endl;
			//保存された辞書データを読み込む
			//フラグを設定する
			//...
			break;
		case 'b':
			std::cout << "認識を停止する" << std::endl;
			processFlag = PROCESS_NONE;
			break;
		}

		/* 
		switch(processFlag)
		{
		case PROCESS_SAVEPOSE1:
		.....		
		}
		*/
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

void KinectControl::setRgbImage()
{
	// RGBカメラのフレームデータを取得する
	NUI_IMAGE_FRAME imageFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		imageStreamHandle,0,&imageFrame));

	//画像取得
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect(0,&colorData,0,0);

	//画像コピー
	rgbIm = cv::Mat(height,width,CV_8UC4,colorData.pBits);
	cv::cvtColor(rgbIm,rgbIm,CV_BGRA2BGR);

	//フレーム解放
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
		imageStreamHandle,&imageFrame));
}

void KinectControl::setDepthImage()
{
	NUI_IMAGE_FRAME depthFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		depthStreamHandle, 0, &depthFrame));
	
	//距離画像取得
	NUI_LOCKED_RECT depthData = {0};
	depthFrame.pFrameTexture->LockRect(0, &depthData, 0, 0);

	USHORT* depth = (USHORT*)depthData.pBits;
	
	depthIm = cv::Mat(height, width, CV_16UC1, depth);
	double minVal, maxVal;
	cv::minMaxIdx(depthIm, &minVal, &maxVal);
	depthIm.convertTo(depthIm, CV_8UC1, 255.0/maxVal - minVal);	

	setPlayerIndex(depth);

	//フレーム解放
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
		depthStreamHandle,&depthFrame));
}

void KinectControl::setPlayerIndex(USHORT* depth)
{
	playerIm = cv::Mat::zeros(height, width, CV_8UC1);
	int i = 0;
	int y = 0;
	int x = 0;
	for (y=0; y<height; y++)
	{
		for (x=0; x<width; x++)
		{
			USHORT distance = ::NuiDepthPixelToDepth(depth[i]);
			USHORT player = ::NuiDepthPixelToPlayerIndex( depth[i] );

			LONG colorX = 0;
			LONG colorY = 0;
			kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, x, y, depth[i], &colorX, &colorY);
			if (player != 0) {
				colorX = (colorX >= width) ? width-1 : (colorX<0 ? 0 : colorX);
				colorY = (colorY >= height) ? height-1 : (colorY<0 ? 0 : colorY);
				playerIm.at<UCHAR>(colorY,colorX) = 255;
			}
			i++;
		}
	}
}

void KinectControl::setSkeletonImage()
{
	NUI_SKELETON_FRAME skeletonFrame = {0};
	kinect->NuiSkeletonGetNextFrame(0, &skeletonFrame);

	skeletonIm = cv::Mat::zeros(height,width,CV_8UC3);
	rgbIm.copyTo(skeletonIm,playerIm);
	for (int i=0; i < NUI_SKELETON_COUNT; i++) {
//		const NUI_SKELETON_DATA &skeleton = skeletonFrame.SkeletonData[i]; //DONE
 
		switch (skeleton.eTrackingState)
		{
		case NUI_SKELETON_TRACKED: //詳細スケルトンデータを得られる
			drawTrackedSkeleton(skeletonIm, skeleton);
//			trackedSkeleton = skeleton; //DONE
			break;
 
		case NUI_SKELETON_POSITION_ONLY: //重心だけ
			drawPoint(skeletonIm, skeleton.Position);
			break;
		}
		if(skeleton.eTrackingState == NUI_SKELETON_TRACKED) break;
	}
}

void KinectControl::drawTrackedSkeleton(cv::Mat& image, const NUI_SKELETON_DATA& skeleton)
{
	// 胴体
	drawBone(image, skeleton, NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
	drawBone(image, skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
	drawBone(image, skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);
	drawBone(image, skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
	drawBone(image, skeleton, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);

	// 腕や足などの描画
	// .........
	// .........
}

void KinectControl::drawBone(cv::Mat& image, const NUI_SKELETON_DATA & skeleton, NUI_SKELETON_POSITION_INDEX jointFrom, NUI_SKELETON_POSITION_INDEX jointTo)
{
	NUI_SKELETON_POSITION_TRACKING_STATE jointFromState = skeleton.eSkeletonPositionTrackingState[jointFrom];
	NUI_SKELETON_POSITION_TRACKING_STATE jointToState = skeleton.eSkeletonPositionTrackingState[jointTo];
	
	// 追跡されたポイントのみを描く
	if ((jointFromState == NUI_SKELETON_POSITION_INFERRED || jointToState == NUI_SKELETON_POSITION_INFERRED) ||
		(jointFromState == NUI_SKELETON_POSITION_TRACKED && jointToState == NUI_SKELETON_POSITION_TRACKED))
	{
		const Vector4 jointFromPosition = skeleton.SkeletonPositions[jointFrom];
		const Vector4 jointToPosition = skeleton.SkeletonPositions[jointTo];
		drawLine(image, jointFromPosition, jointToPosition);
	}
}

void KinectControl::drawLine( cv::Mat& image, Vector4 pos1, Vector4 pos2)
{
	// ３次元の位置から距離画像での位置に変換
	FLOAT depthX1 = 0, depthY1 = 0;
	FLOAT depthX2 = 0, depthY2 = 0;

	NuiTransformSkeletonToDepthImage(pos1, &depthX1, &depthY1, CAMERA_RESOLUTION);
	NuiTransformSkeletonToDepthImage(pos2, &depthX2, &depthY2, CAMERA_RESOLUTION);

	// 距離画像での位置からRGB画像での位置に変換
	LONG colorX1 = 0, colorY1 = 0;
	LONG colorX2 = 0, colorY2 = 0;
	kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
		CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, (LONG)depthX1, (LONG)depthY1, 0, &colorX1, &colorY1 );
	kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
		CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, (LONG)depthX2, (LONG)depthY2, 0, &colorX2, &colorY2 );
	
	// RGB画像での位置に線分を描画
	cv::line(image, cv::Point(colorX1,colorY1), cv::Point(colorX2,colorY2), cv::Scalar(50,255,50), 5);
}


void KinectControl::drawPoint( cv::Mat& image, Vector4 position )
{
	// ３次元の位置から距離画像での位置に変換
	FLOAT depthX = 0, depthY = 0;
	NuiTransformSkeletonToDepthImage(position, &depthX, &depthY, CAMERA_RESOLUTION);

	// 距離画像での位置からRGB画像での位置に変換
	LONG colorX = 0;
	LONG colorY = 0;
	kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
		CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0,
		(LONG)depthX, (LONG)depthY, 0, &colorX, &colorY );
	
	// RGB画像での位置に丸を描画
	cv::circle( image, cv::Point(colorX,colorY), 10, cv::Scalar( 0, 255, 0), 5);
}
