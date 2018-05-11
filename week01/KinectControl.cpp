#include "KinectControl.h"

#define USE_RGB   1
#define USE_DEPTH 1
#define USE_NEAR  1

//�J���[�p���b�g
cv::Vec4b blue   = cv::Vec4b(255,   0,   0, 0);
cv::Vec4b green  = cv::Vec4b(  0, 255,   0, 0);
cv::Vec4b red    = cv::Vec4b(  0,   0, 255, 0);
cv::Vec4b yellow = cv::Vec4b(255, 255,   0, 0);
cv::Vec4b orange = cv::Vec4b(  0, 255, 255, 0);
cv::Vec4b purple = cv::Vec4b(255,   0, 255, 0);
cv::Vec4b black  = cv::Vec4b(  0,   0,   0, 0);

KinectControl::KinectControl(){}

KinectControl::~KinectControl(){
	//�I������
	if(kinect != 0){
		kinect->NuiShutdown();
		kinect->Release();
	}
}

void KinectControl::initialize(){
	createInstance();

	//Kinect�̏����ݒ�
	ERROR_CHECK(kinect->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX));

	#if USE_RGB //RGB������
		ERROR_CHECK(kinect->NuiImageStreamOpen(
			NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION, 0, 2, 0, &imageStreamHandle));
	#endif

	#if USE_DEPTH //Depth������
		ERROR_CHECK(kinect->NuiImageStreamOpen(
			NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION, 0, 2, 0, &depthStreamHandle));
	#endif //CAMERA_RESOLUTION��KinectControl.h�Œ�`����Ă���(640x480)

	#if USE_NEAR //Near���[�h���g�p����ɂ́ANear���[�h�̃t���O�������ɐݒ肷��
		ERROR_CHECK(kinect->NuiImageStreamSetImageFrameFlags(
			depthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE));
	#endif

	//�t���[���X�V�C�x���g�̍쐬
	streamEvent = ::CreateEventA(0, TRUE, FALSE, 0);
	ERROR_CHECK(kinect->NuiSetFrameEndEvent(streamEvent, 0));

	::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height);
}

void KinectControl::run(){
	LONG angle = 0;  //�p�x
	LONG x = 0;  //���݂̊p�x�i�̂��Ɏ擾�j
	kinect->NuiCameraElevationSetAngle(angle);
	cv::Mat my_image = cv::Mat::zeros(height, width, CV_8UC4);
	//���C�����[�v
	while(1){
		//�X�V�҂�
		DWORD ret = ::WaitForSingleObject(streamEvent, INFINITE);
		::ResetEvent(streamEvent);

		//�J���[�摜�擾�ƕ\��
		this->setRgbImage(this->rgbIm);
		cv::imshow("RGB Image", this->rgbIm);

		//�����摜�擾�ƕ\���܂��̓��[�U�[�C���f�b�N�X�̕\��
		this->setDepthImage(this->depthIm);
		cv::imshow("My Image", my_image);

		//�L�[�E�F�C�g
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
		else if(key == 'r'){//�F���[�e�[�V����
			colorflag = 1;

		}
		else if(key == 'x'){//�w�i�ƃO��
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
	//Kinect�̐����擾
	int count = 0;
	ERROR_CHECK(::NuiGetSensorCount(&count));
	if(count == 0){
		throw std::runtime_error("Kinect��ڑ����Ă�������");
	}

	//�ŏ��̃C���X�^���X�쐬
	ERROR_CHECK(::NuiCreateSensorByIndex(0, &kinect));

	//Kinect�̏�Ԃ��擾
	HRESULT status = kinect->NuiStatus();
	if(status != S_OK){
		throw std::runtime_error("Kinect�����p�\�ł͂���܂���");
	}
}

void KinectControl::setRgbImage(cv::Mat& image){
	// RGB�J�����̃t���[���f�[�^���擾����
	NUI_IMAGE_FRAME imageFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		imageStreamHandle, 0, &imageFrame));

	//�摜�擾
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect(0, &colorData, 0, 0);

	//�摜�R�s�[
	image = cv::Mat(height, width, CV_8UC4, colorData.pBits);

	//�t���[�����
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
		imageStreamHandle, &imageFrame));
}

void KinectControl::setDepthImage(cv::Mat& image){
	//�����摜�̃t���[���f�[�^���擾����
	NUI_IMAGE_FRAME depthFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		depthStreamHandle, 0, &depthFrame));


	//�����摜�擾
	NUI_LOCKED_RECT depthData = {0};
	depthFrame.pFrameTexture->LockRect(0, &depthData, 0, 0);

	//
	USHORT* depth = (USHORT*)depthData.pBits;
	cv::Mat(height, width, CV_16UC1, depth).convertTo(image, CV_8UC4, 1/32.0);

	//���[�U�[�C���f�b�N�X���擾
	this->setPlayerIndex(this->playerIm, depth);

	//�t���[������Y��Ȃ��悤��
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
		depthStreamHandle, &depthFrame));
}

void KinectControl::setPlayerIndex(cv::Mat& image, USHORT* depth){
	//depth: �S��f�̃f�v�X�f�[�^�i�����l+�l�����j

	//���[�U�[�C���f�b�N�X�摜�̏���(image�ϐ��̏�����)(http://opencv.jp/cookbook/opencv_mat.html)
	image = cv::Mat::zeros(height, width, CV_8UC4);

	int i = 0; //depth�ɉ�f���Ƃ̃C���f�b�N�X
	for(int y = 0; y < this->height; y++){
		for(int x = 0; x < this->width; x++){
			i = width*y + x;

			//NuiDepthPixelToDepth��NuiDepthPixelToPlayerIndex�g���Ƌ����f�[�^�ƃ��[�U�[�C���f�b�N�X���擾�ł���
			USHORT distance = NuiDepthPixelToDepth(depth[i]);
			USHORT player   = NuiDepthPixelToPlayerIndex(depth[i]);

			//�����摜���W����J���[�摜���@�ɕϊ�����
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
