#define _CRT_SECURE_NO_WARNINGS

//ffmpeg�����
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#pragma  comment(lib,"avcodec.lib")
#pragma  comment(lib,"avutil.lib")
#pragma  comment(lib,"swscale.lib")
};

//opencv�����
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/imgproc/imgproc_c.h"
#pragma comment(lib,"opencv_core2413.lib")
#pragma comment(lib,"opencv_highgui2413.lib")
#pragma comment(lib,"opencv_imgproc2413.lib") 

#include <atlstr.h>
/*
	����ϵͳͷ�ļ�
*/
#include <windows.h>
#include <winnt.h>
#include <string>

/*
	���ù���ͷ�ļ�
*/
#include "Video.h"

#define HQVT_NET_IMPORT		//ʹ��HQVT �汾�����
#pragma comment(lib, "HH5PlayerSDK.lib")
#ifdef HQVT_NET_IMPORT
#pragma comment(lib, "SNetapi.lib")
#else
#pragma comment(lib, "HHNetClient.lib")
#endif
#include "HHAVDefine.h"
#ifdef HQVT_NET_IMPORT
#include "SNetapi.h"
#else
#include "HHNetInterface.h"
#endif
#include "HH5PlayerSDK.h"
#include "Hqvt_face_Def.h"


//�Ƿ�¼����Ƶ

/*�������ݽṹ*/
struct internal_handle
{
	StreamCallback OnStreamReady;/*ʵʱ��Ƶ���ص�*/
	RGBCallback OnRealRGBReady;  /*ʵʱ����ͼƬ�ص�*/
	RGBCallback OnTrigRGBReady;  /*�ⲿ����ץ��ͼƬ�ص�*/
	TrigCallback OnTrigRGBReadyEx;  /*�ⲿ����ץ��ͼƬ����Ϣ�ص�*/

	//�������
	HANDLE hVideoChannel;

	bool  bRealCallback;
	bool  bTrigCallback;
	void* pUserData;
	SwsContext* pCtx;
	char* pRGBBuffer;
	long lUserHandle; //�û����    
	long lRealHandle; //ʵʱ�����	
	long lDecodePort; //����ͨ����
	LONG lAlarmHandle;//�����������
	long lChannel;
	long lCount;
	bool Gateflag;  //��բ״̬ true����  false:��
	int nCameraID;  //���ID
	//CvxText *pText;
	//	CVehicleDetect *pVehicleDetect;
	internal_handle()
	{
		OnStreamReady = NULL;
		OnRealRGBReady = NULL;
		OnTrigRGBReady = NULL;
		OnTrigRGBReadyEx = NULL;
		bRealCallback = false;
		bTrigCallback = false;
		pUserData = NULL;
		pCtx = NULL;
		pRGBBuffer = NULL;
		lUserHandle = 0; //�û����    
		lRealHandle = 0; //ʵʱ�����	
		lDecodePort = 0; //����ͨ����
		lAlarmHandle = 0;//�����������
		lChannel = 0;
		lCount = 0;
		//	pText = NULL;
		//	pVehicleDetect = NULL;
		Gateflag = false;
	}
};

///*����ȫ�ֱ��������ID*/
//int g_nCameraID = 0;
///*����ȫ�ֱ������û�����*/
//int g_nInternal_UserNum = 0;

/*��ע����ʱ�����*/
//typedef struct _IpkeyPare_
//{
//	internal_handle *ph;
//	int nID;
//}IpkeyPare;
//
////����û�����������
//#define KEY_PARE_NUM 10
//
//IpkeyPare g_pPare[KEY_PARE_NUM];


//��ȡ�����ļ�·��
std::string GetModuleFilePath()
{
	std::string str;
	CHAR szFile[255] = { 0 };
	int len = 0;
	DWORD dwRet = GetModuleFileName(NULL, szFile, 255);
	if (dwRet != 0)
	{
		str = (szFile);
		int nPos = (int)str.rfind('\\');
		if (nPos != -1)
		{
			str = str.replace(nPos, str.length(), "\\YTControl.ini");
		}
	}
	return str;
}

//ת��
IplImage *LoadImageNet(char *pBuffer, int nBufferSize)
{
	if (pBuffer == NULL || nBufferSize == 0)
		return NULL;

	CvMat mat = cvMat(2000, 1500, CV_8UC1, pBuffer);
	IplImage *p = cvDecodeImage(&mat, 1);//����

	if (p != NULL)
		return p;
	else
		return NULL;
}

/*��ͼ�����ص�����*/
int CapPicCallBack(HANDLE hPic, void* pPicData, int nPicLen, DWORD dwClientID, void* pContext)
{
	internal_handle* pih = (internal_handle*)pContext;
	if (pih == NULL)
		return 0;
	if (!pih->bTrigCallback)
		return 0;
	HH_PICTURE_INFO sPicInfo;
	memset(&sPicInfo, '\0', sizeof(HH_PICTURE_INFO));
	SNET_ReadPictureInfo(hPic, sPicInfo);
	//���� = ͼƬ���� + ͼ����Ϣ(VEHICLE_INFO_S)
	FACE_PICS_INFO_S stImgInfo = { 0 };
	memcpy((char*)&stImgInfo, (char*)pPicData, sizeof(FACE_PICS_INFO_S));

	//��ȡ����·��
	std::string strPath = GetModuleFilePath();

	CString strPathCS = strPath.c_str();

	if (stImgInfo.dwBgnFlag == 0x12345678 && stImgInfo.dwEndFlag == 0x87654321)
	{
		//��ȡ�ļ�·��
		CString strFile;
		strPathCS = strPathCS.Left(strPathCS.ReverseFind('\\'));
		strPathCS += "\\Picture";
		DWORD dwDataLen = 0;	//���ݳ���
		for (int inx = 0; inx < stImgInfo.byPicNum; ++inx)
		{
			time_t timestamp = stImgInfo.stPics[inx].u64CapTime;
			tm* ttime = { 0 };
			ttime = gmtime(&timestamp);

			//����ͼƬ����
			if (stImgInfo.stPics[inx].byImageType = 0x01)
			{
				strFile.Format("%s\\%02d%02d%02d_%d_target.%s", strPathCS, ttime->tm_hour, ttime->tm_min, ttime->tm_sec,
					stImgInfo.dwTrackID, (sPicInfo.picFormatType == 0) ? "jpg" : "bmp");
			}
			else if (stImgInfo.stPics[inx].byImageType = 0x02)
			{
				strFile.Format("%s\\%02d%02d%02d_%d_face.%s", strPathCS, ttime->tm_hour, ttime->tm_min, ttime->tm_sec,
					stImgInfo.dwTrackID, (sPicInfo.picFormatType == 0) ? "jpg" : "bmp");
			}
			else if (stImgInfo.stPics[inx].byImageType = 0x04)
			{
				strFile.Format("%s\\%02d%02d%02d_%d_body.%s", strPathCS, ttime->tm_hour, ttime->tm_min, ttime->tm_sec,
					stImgInfo.dwTrackID, (sPicInfo.picFormatType == 0) ? "jpg" : "bmp");
			}

			int nDataLen1 = stImgInfo.stPics[inx].dwPicLen;
			if (nDataLen1 > 0)
			{
				/*std::ofstream ofs(strFile, std::ofstream::out);
				ofs.write((char *)pPicData + sizeof(FACE_PICS_INFO_S)+dwDataLen, nDataLen1);
				ofs.close();
				dwDataLen += nDataLen1;*/

				IplImage *pImage = LoadImageNet((char*)pPicData + sizeof(FACE_PICS_INFO_S)+dwDataLen, nDataLen1);
				if (pImage == NULL)
					return -1;

				if (pih->OnTrigRGBReadyEx != NULL)
				{
					//NET_DVR_PLATE_INFO plateInfo;
					//plateInfo = plateResult.struPlateInfo;
					//memcpy(&plateInfo.struPlateRect, &plateResult.struPicInfo[0].struPlateRect, sizeof(plateInfo.struPlateRect));
					//memcpy(plateInfo.byRes, &plateResult.struVehicleInfo.wSpeed, sizeof(plateResult.struVehicleInfo.wSpeed));
					//*(WORD*)&plateInfo.byRes[0] = plateResult.struVehicleInfo.wSpeed;
					pih->OnTrigRGBReadyEx(pih->pUserData, (void*)pImage->imageData, pImage->width, 0, pImage->height, NULL, 0, 0);
					cvReleaseImage(&pImage);
				}

				FILE *fp = fopen(strFile, "wb");
				fwrite((char*)pPicData + sizeof(FACE_PICS_INFO_S)+dwDataLen, sizeof(char), nDataLen1, fp);
				fclose(fp);
				dwDataLen += nDataLen1;
			}
		}
	}

	return 0;
}


//---------------------------------------------------------------------------------------------------------------------
//ʵʱ����Ƶ���ݻص�
//---------------------------------------------------------------------------------------------------------------------
int	WINAPI RealStreamCallback(HANDLE hOpenChannel,void *pStreamData,	DWORD dwClientID,void *pContext,ENCODE_VIDEO_TYPE encodeVideoType,HHAV_INFO *pAVInfo)
{
	internal_handle *pih = (internal_handle*)pContext;   //���������
	if (pih == NULL)
		return 0;
	if (!pih->bRealCallback)
		return 0;
	DWORD			dwFrameSize = 0;
	HV_FRAME_HEAD	*pFrameHead = (HV_FRAME_HEAD *)pStreamData;
	EXT_FRAME_HEAD	*pExtFrameHead = (EXT_FRAME_HEAD *)((char*)pFrameHead + sizeof(HV_FRAME_HEAD));
	int				 ret = 0;

	dwFrameSize = sizeof(HV_FRAME_HEAD)+pFrameHead->nByteNum;
	/*if (pFrameHead->streamFlag != FRAME_FLAG_A)
	{
		g_nVideoWidth = pExtFrameHead->szFrameInfo.szFrameVideo.nVideoWidth;
		g_nVideoHeight = pExtFrameHead->szFrameInfo.szFrameVideo.nVideoHeight;
	}*/

#if 0
	if (g_pFaceDlg->m_bRecord)
	{
		if (g_pFaceDlg->m_pfRecord == NULL)
		{
			CString strFileName;
			CTime pt = CTime::GetCurrentTime();
			strFileName.Format("%02d_%02d_%02d.dat", pt.GetHour(), pt.GetHour(), pt.GetSecond());
			g_pFaceDlg->m_pfRecord = fopen(strFileName, "wb+");
		}
		if (g_pFaceDlg->m_pfRecord)
		{
			int nsize = sizeof(HV_FRAME_HEAD)+sizeof(EXT_FRAME_HEAD);

			fwrite((char*)pStreamData + nsize, pFrameHead->nByteNum, 1, g_pFaceDlg->m_pfRecord);
		}
	}
	else
	{
		if (g_pFaceDlg->m_pfRecord != NULL)
		{
			fclose(g_pFaceDlg->m_pfRecord);
			g_pFaceDlg->m_pfRecord = NULL;
		}
	}
#endif
	if (pih->hVideoChannel != NULL)
	{
		/*if (g_bIsWindowVisable)
		{*/
		//ret = HH5PLAYER_PutDecStreamDataEx(wDisplayWnd, (BYTE *)pStreamData, dwFrameSize, encodeVideoType, (HH5KAV_INFO *)pAVInfo);
		/*}
		else
		{*/
		/*unsigned char nFrameType = pFrameHead->streamFlag;
		unsigned char  nDataType = encodeVideoType;*/
		//HH5PLAYER_PutDecStreamDataEx(36, (BYTE *)pStreamData, dwFrameSize);
		//}

		LARGE_INTEGER litmp;
		_int64 QPart1, QPart2;
		double dfMinus, dfFreq, dfTim;
		QueryPerformanceFrequency(&litmp);

		dfFreq = (double)litmp.QuadPart;
		QueryPerformanceCounter(&litmp);
		QPart1 = litmp.QuadPart;

		//��ʾӰ��
		//SYSTEMTIME stOld, stNew;
		//GetLocalTime(&stOld);
		pih->OnStreamReady(pih->pUserData, (void*)pStreamData, dwFrameSize, encodeVideoType);

		QueryPerformanceCounter(&litmp);
		QPart2 = litmp.QuadPart;
		dfMinus = (double)(QPart2 - QPart1);
		dfTim = dfMinus / dfFreq * 1000;

		//��ʾʱ��
		CString msg4 = "ʱ�䣺", msg3, msg5 = "����";
		msg3.Format("%10.9f", dfTim);
		CString st = msg4 + msg3 + msg5;
		//GetLocalTime(&stNew);
	}

	return ret;
}


/*
	�������ƣ�InitialVideoSource
	����������������
			  ʵʱ��Ƶ���ص�
	          ʵʱ����ͼƬ�ص�
			  �ⲿ����ץ��ͼƬ�ص�
	����˵������ʼ����ƵԴ������ص����ڻ�ȡ�������ⲿ����ץ�ĵ�ͼƬ
*/
VIDEO_API handle __cdecl  InitialVideoSource(VideoParam *param, StreamCallback OnStreamReady, RGBCallback OnRealRGBReady, RGBCallback OnTrigRGBReady,void *pUserData)
{
	//��ʼ���ṹ��ָ��
	internal_handle *pih = new internal_handle;
	pih->OnStreamReady = OnStreamReady;
	pih->OnRealRGBReady = OnRealRGBReady;
	pih->OnTrigRGBReady = OnTrigRGBReady;
	//pih->bRealCallback = true;
	pih->pUserData = pUserData;
	pih->pRGBBuffer = NULL;
	pih->pCtx = NULL;
	pih->lRealHandle = 0;
	pih->lChannel = param->AddressNum;

	std::string strDevName = "";
	HHERR_CODE errCode;
	HHOPEN_CHANNEL_INFO_EX openInfo;
	char bufferUserPwd[20] = "";
	memset(bufferUserPwd, '\0', 20);
	//��ȡ�û���������
	char bufferUserName[20] = { 0 };
	memset(bufferUserName, 0, 20);
	std::string strPath = GetModuleFilePath();
	
	//��ȡ�˻�����
	::GetPrivateProfileString("YTControl", "UserName", "admin", bufferUserPwd, MAX_PATH, strPath.c_str());
	::GetPrivateProfileString("YTControl", "PassWord", "admin", bufferUserName, MAX_PATH, strPath.c_str());
	int iStreamType = ::GetPrivateProfileInt("YTControl", "StreamType", 1, strPath.c_str()); //��������� 0:������ 1:������

	openInfo.dwClientID = 0;
	openInfo.nOpenChannel = 0;
	openInfo.nSubChannel = 0;
	openInfo.protocolType = NET_PROTOCOL_TCP;
	openInfo.funcStreamCallback = RealStreamCallback;
	openInfo.pCallbackContext = (void*)pih;

	errCode = SNET_OpenChannel(
		(char *)(const char *)(param->CameraIp),
		param->CameraPort,
		(char *)(const char *)strDevName.c_str(),
		(char *)(const char *)bufferUserName,
		(char *)(const char *)bufferUserPwd,
		(HHOPEN_CHANNEL_INFO *)&openInfo,
		pih->hVideoChannel);

	return (handle)pih;
}


/*
�������ƣ�InitialVideoSource3
��������������Ϣ
		  ʵʱ��Ƶ���ص�
		  ������������ص�
		  ʵʱ����ͼƬ�ص�
		  �ⲿ����ץ��ͼƬ�ص�
		  �ⲿ����ץ��ͼƬ�ص�
����˵������ʼ����ƵԴ������ص����ڻ�ȡ�������ⲿ����ץ�ĵ�ͼƬ
*/
VIDEO_API handle __cdecl  InitialVideoSource3(VideoParam *param, StreamCallback OnStreamReady, PixBufCallback OnPixBufReady, RGBCallback OnRealRGBReady, RGBCallback OnTrigRGBReady, TrigCallback OnTrigRGBReadyEx, void *pUserData)
{
	internal_handle *pih = NULL;
	char bufferUserPwd[20] = "";
	memset(bufferUserPwd, '\0', 20);
	//��ȡ�û���������
	char bufferUserName[20] = { 0 };
	memset(bufferUserName, 0, 20);
	std::string strPath = GetModuleFilePath();
	//��ȡ�˻�����
	::GetPrivateProfileString("YTControl", "UserName", "admin", bufferUserPwd, MAX_PATH, strPath.c_str());
	::GetPrivateProfileString("YTControl", "PassWord", "admin", bufferUserName, MAX_PATH, strPath.c_str());
	pih = (internal_handle*)InitialVideoSource(param, OnStreamReady, OnRealRGBReady, OnTrigRGBReady, pUserData);
	if (NULL != pih)
	{
		pih->OnTrigRGBReadyEx = OnTrigRGBReadyEx;

		if (pih->OnTrigRGBReadyEx)
		{
			HHERR_CODE errCode;
			HHOPEN_PICTURE_INFO sOpenPicInfo;
			sOpenPicInfo.dwClientID = 0;
			sOpenPicInfo.nOpenChannel = 0;
			sOpenPicInfo.protocolType = NET_PROTOCOL_TCP;
			sOpenPicInfo.funcPictureCallback = CapPicCallBack;
			sOpenPicInfo.pCallbackContext = (void*)pih;

			errCode = SNET_OpenPicture(
				(char *)(const char *)(param->CameraIp),
				param->CameraPort,
				(char *)(const char *)"",
				(char *)(const char *)bufferUserName,
				(char *)(const char *)bufferUserPwd,
				(HHOPEN_PICTURE_INFO *)&sOpenPicInfo, 
				pih->hVideoChannel);
		}
	}
	return (handle)pih;
}


/*��ȡ��ƵԴ��Ϣ������0ʧ�ܣ�1�ɹ�*/
VIDEO_API int   __cdecl  GetVideoInfo(handle hVS, int *decode, int *width, int *height, int *codec)
{
	internal_handle *pih = (internal_handle*)hVS;

	*decode = 0; //0-�ⲿ���룬1-�ڲ�����
	*width = 0;
	*height = 0;
	*codec = AV_CODEC_ID_H264;

	return 1;
}

/*������ƵԴ������0ʧ�ܣ�1�ɹ�*/
VIDEO_API int    __cdecl  StartVideoSource(handle hVS)
{
	internal_handle *pih = (internal_handle*)hVS;
	pih->bRealCallback = true;
	pih->bTrigCallback = true;
	return 1;
}

/*��ͣ��ƵԴ�����øú�������Ȼ���Ե���StartVideoSource����*/
VIDEO_API void   __cdecl  StopVideoSource(handle hVS)
{
	internal_handle *pih = (internal_handle*)hVS;
	pih->bRealCallback = false;
	pih->bTrigCallback = false;
}

/*��ֹ��ƵԴģ��*/
VIDEO_API void   __cdecl  FinializeVideoSource(handle hVS)
{
	internal_handle *pih = (internal_handle*)hVS;

	if (pih->hVideoChannel)
	{
		SNET_CloseChannel(pih->hVideoChannel);
		pih->hVideoChannel = NULL;
		Sleep(500);   //�ӳ�500,�ȴ����̹߳ҵ�
	}

	//HH5PLAYER_ReleasePlayer(0);

	SNET_ClosePicture(NULL);

	SNET_LogoffServer(NULL);

	SNET_Cleanup();

	if (pih->pCtx != NULL)
	{
		sws_freeContext(pih->pCtx);
	}
	if (pih->pRGBBuffer != NULL)
	{
		delete[] pih->pRGBBuffer;
	}

	delete pih;
	pih = NULL;
}