#include "KinectControl.h"


KinectControl::KinectControl()
{
}

KinectControl::~KinectControl()
{
	//�I������
	if(kinect != 0){
		kinect->NuiShutdown();
		kinect->Release();
	}
}


void KinectControl::initialize()
{
	createInstance();

	//Kinect�̏����ݒ�
	/* �����摜���g�p�ł���悤�L�q��ǉ����� DONE*/
	ERROR_CHECK(kinect->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX));

	//RGB������ DONE Depth���g���Ƃ��͂�������R�����g�A�E�g�����fps���悭�Ȃ�
//	ERROR_CHECK(kinect->NuiImageStreamOpen(
//		NUI_IMAGE_TYPE_COLOR,CAMERA_RESOLUTION,0,2,0,&imageStreamHandle));

	//Depth������
	/* �������L�q DONE*/
	ERROR_CHECK(kinect->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,CAMERA_RESOLUTION,0,2,0,&depthStreamHandle));


	//Near���[�h���g�p����ɂ́ANear���[�h�̃t���O�������ɐݒ肷�� DONE/
	ERROR_CHECK(kinect->NuiImageStreamSetImageFrameFlags(
		depthStreamHandle,NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE));

	//�t���[���X�V�C�x���g�̍쐬
	streamEvent = ::CreateEventA(0,TRUE,FALSE,0);
	ERROR_CHECK(kinect->NuiSetFrameEndEvent(streamEvent,0));

	::NuiImageResolutionToSize(CAMERA_RESOLUTION,width,height);
}

void KinectControl::run(){
	//���C�����[�v
	while(1){
		//�X�V�҂�
		DWORD ret = ::WaitForSingleObject(streamEvent,INFINITE);
		::ResetEvent(streamEvent);

		//�J���[�摜�擾�ƕ\��
//		this->setRgbImage(this->rgbIm);
//		cv::imshow("RGB Image",this->rgbIm);

		//�����摜�擾�ƕ\���܂��̓��[�U�[�C���f�b�N�X�̕\��
		/* �������L�q DONE*/
//		this->setDepthImage(this->depthIm);
//		cv::imshow("Depth Image",this->depthIm);

		this->setPlayerIndex(this->playerIm);
		this->smoothing(this->playerIm);
		cv::imshow("Player Image",this->playerIm);

		//�L�[�E�F�C�g
		int key = cv::waitKey(10);
		if(key == 'q'){
			break;
		}
	}
}

void KinectControl::createInstance()
{
	//Kinect�̐����擾
	int count = 0;
	ERROR_CHECK(::NuiGetSensorCount(&count));
	if(count == 0){
		throw std::runtime_error("Kinect��ڑ����Ă�������");
	}

	//�ŏ��̃C���X�^���X�쐬
	ERROR_CHECK(::NuiCreateSensorByIndex(0,&kinect));

	//Kinect�̏�Ԃ��擾
	HRESULT status = kinect->NuiStatus();
	if(status!=S_OK){
		throw std::runtime_error("Kinect�����p�\�ł͂���܂���");
	}
}

void KinectControl::setRgbImage(cv::Mat& image)
{
	// RGB�J�����̃t���[���f�[�^���擾����
	NUI_IMAGE_FRAME imageFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		imageStreamHandle,0,&imageFrame));

	//�摜�擾
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect(0,&colorData,0,0);

	//�摜�R�s�[
	image = cv::Mat(height,width,CV_8UC4,colorData.pBits);

	//�t���[�����
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
		imageStreamHandle,&imageFrame));
}

void KinectControl::setDepthImage(cv::Mat& image)
{
	//�����ɋ����摜�擾�̂��߂̏���������
	//�����摜�̃t���[���f�[�^���擾����(setRgbImage���Q�l�ɂ���)
	/* �������L�q DONE*/
	NUI_IMAGE_FRAME depthFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		depthStreamHandle,0,&depthFrame));

	
	//�����摜�擾
	NUI_LOCKED_RECT depthData = {0};
	/* �������L�q DONE*/
	depthFrame.pFrameTexture->LockRect(0,&depthData,0,0);


	USHORT* depth = (USHORT*)depthData.pBits;
	/* �������L�q DONE*/
	//13bit���̏��8bit����肽���̂�5bit�E�V�t�g����
	cv::Mat(height,width,CV_16U,depth).convertTo(image,CV_8U,1/32.0);
	


	//���[�U�[�C���f�b�N�X���擾�ꍇ�AsetPlayerIndex�֐��������ɌĂяo���܂��傤
	//this->setPlayerIndex(this->playerIm, depth);

	//�t���[������Y��Ȃ��悤��
	/* �������L�q DONE*/
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
			depthStreamHandle, &depthFrame));
}

void KinectControl::smoothing(cv::Mat& image){
	cv::dilate(image, image, cv::Mat(), cv::Point(-1, -1), 3);
	cv::erode(image, image, cv::Mat(), cv::Point(-1, -1), 3);
}


void KinectControl::setPlayerIndex(cv::Mat& image) {
	//depth: �S��f�̃f�v�X�f�[�^�i�����l+�l�����j

	NUI_IMAGE_FRAME depthFrame = {0};
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(
		depthStreamHandle,0,&depthFrame));

	NUI_LOCKED_RECT depthData = {0};
	/* �������L�q DONE*/
	depthFrame.pFrameTexture->LockRect(0,&depthData,0,0);

	USHORT* depth = (USHORT*)depthData.pBits;
	
	//���[�U�[�C���f�b�N�X�摜�̏���(image�ϐ��̏�����)
	//OpenCV��cv::Mat::zeros�Q�l�������� (http://opencv.jp/cookbook/opencv_mat.html)
	image = cv::Mat::zeros(height, width, CV_8U);
	/* �������L�q */


	int i = 0; //depth�ɉ�f���Ƃ̃C���f�b�N�X
	int y = 0;
	int x = 0;
	for (y=0; y<this->height; y++) {
		for (x=0; x<this->width; x++) {
			i = width*y + x;
			//NuiDepthPixelToDepth��NuiDepthPixelToPlayerIndex�g���Ƌ����f�[�^�ƃ��[�U�[�C���f�b�N�X���擾�ł���

			USHORT distance = NuiDepthPixelToDepth(depth[i]);
		    USHORT player = ::NuiDepthPixelToPlayerIndex(depth[i]);

			//�����摜���W����J���[�摜���@�ɕϊ�����
			LONG colorX = 0;
			LONG colorY = 0;
			kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(CAMERA_RESOLUTION, CAMERA_RESOLUTION, 
				0, x, y, depth[i], &colorX, &colorY);

			if (player != 0) image.at<UCHAR>(colorY, colorX) = 100*player;
			//�����܂łœ_(colorX,colorY)�ɂ����鋗���l�Ɛl����񂪓���ꂽ���ƂɂȂ�D
			//�ȉ��ȗ�
			//�Ⴆ��if���ɂ��l���̈�݂̂ɐF�t����J���[�摜�̉�f�l�����蓖�Ă��肷��D
			//(���̂Ƃ��V���ɐl���\���p�̍s��(cv::Mat)���쐬����Ƃ悢)
			
			/* �������L�q */
		}
	}

	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(
			depthStreamHandle, &depthFrame));
}
