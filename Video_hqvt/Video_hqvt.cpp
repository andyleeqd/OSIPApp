#define _CRT_SECURE_NO_WARNINGS

//ffmpeg库调用
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#pragma  comment(lib,"avcodec.lib")
#pragma  comment(lib,"avutil.lib")
#pragma  comment(lib,"swscale.lib")
};

//opencv库调用
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/imgproc/imgproc_c.h"
#pragma comment(lib,"opencv_core2413.lib")
#pragma comment(lib,"opencv_highgui2413.lib")
#pragma comment(lib,"opencv_imgproc2413.lib") 

#include <atlstr.h>
/*
	引用系统头文件
*/
#include <windows.h>
#include <winnt.h>
#include <string>

/*
	引用工程头文件
*/
#include "Video.h"

#define HQVT_NET_IMPORT		//使用HQVT 版本网络库
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


//是否录制视频

/*定义数据结构*/
struct internal_handle
{
	StreamCallback OnStreamReady;/*实时视频流回调*/
	RGBCallback OnRealRGBReady;  /*实时解码图片回调*/
	RGBCallback OnTrigRGBReady;  /*外部触发抓拍图片回调*/
	TrigCallback OnTrigRGBReadyEx;  /*外部触发抓拍图片及信息回调*/

	//输出参数
	HANDLE hVideoChannel;

	bool  bRealCallback;
	bool  bTrigCallback;
	void* pUserData;
	SwsContext* pCtx;
	char* pRGBBuffer;
	long lUserHandle; //用户句柄    
	long lRealHandle; //实时流句柄	
	long lDecodePort; //解码通道号
	LONG lAlarmHandle;//报警布防句柄
	long lChannel;
	long lCount;
	bool Gateflag;  //道闸状态 true：开  false:关
	int nCameraID;  //相机ID
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
		lUserHandle = 0; //用户句柄    
		lRealHandle = 0; //实时流句柄	
		lDecodePort = 0; //解码通道号
		lAlarmHandle = 0;//报警布防句柄
		lChannel = 0;
		lCount = 0;
		//	pText = NULL;
		//	pVehicleDetect = NULL;
		Gateflag = false;
	}
};

///*定义全局变量，相机ID*/
//int g_nCameraID = 0;
///*定义全局变量，用户数量*/
//int g_nInternal_UserNum = 0;

/*无注释暂时不清楚*/
//typedef struct _IpkeyPare_
//{
//	internal_handle *ph;
//	int nID;
//}IpkeyPare;
//
////最大用户连接数可能
//#define KEY_PARE_NUM 10
//
//IpkeyPare g_pPare[KEY_PARE_NUM];


//获取配置文件路径
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

//转码
IplImage *LoadImageNet(char *pBuffer, int nBufferSize)
{
	if (pBuffer == NULL || nBufferSize == 0)
		return NULL;

	CvMat mat = cvMat(2000, 1500, CV_8UC1, pBuffer);
	IplImage *p = cvDecodeImage(&mat, 1);//解码

	if (p != NULL)
		return p;
	else
		return NULL;
}

/*截图函数回调函数*/
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
	//数据 = 图片数据 + 图像信息(VEHICLE_INFO_S)
	FACE_PICS_INFO_S stImgInfo = { 0 };
	memcpy((char*)&stImgInfo, (char*)pPicData, sizeof(FACE_PICS_INFO_S));

	//获取程序路径
	std::string strPath = GetModuleFilePath();

	CString strPathCS = strPath.c_str();

	if (stImgInfo.dwBgnFlag == 0x12345678 && stImgInfo.dwEndFlag == 0x87654321)
	{
		//获取文件路径
		CString strFile;
		strPathCS = strPathCS.Left(strPathCS.ReverseFind('\\'));
		strPathCS += "\\Picture";
		DWORD dwDataLen = 0;	//数据长度
		for (int inx = 0; inx < stImgInfo.byPicNum; ++inx)
		{
			time_t timestamp = stImgInfo.stPics[inx].u64CapTime;
			tm* ttime = { 0 };
			ttime = gmtime(&timestamp);

			//生成图片名称
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
//实时音视频数据回调
//---------------------------------------------------------------------------------------------------------------------
int	WINAPI RealStreamCallback(HANDLE hOpenChannel,void *pStreamData,	DWORD dwClientID,void *pContext,ENCODE_VIDEO_TYPE encodeVideoType,HHAV_INFO *pAVInfo)
{
	internal_handle *pih = (internal_handle*)pContext;   //播放器编号
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

		//显示影像
		//SYSTEMTIME stOld, stNew;
		//GetLocalTime(&stOld);
		pih->OnStreamReady(pih->pUserData, (void*)pStreamData, dwFrameSize, encodeVideoType);

		QueryPerformanceCounter(&litmp);
		QPart2 = litmp.QuadPart;
		dfMinus = (double)(QPart2 - QPart1);
		dfTim = dfMinus / dfFreq * 1000;

		//显示时间
		CString msg4 = "时间：", msg3, msg5 = "毫秒";
		msg3.Format("%10.9f", dfTim);
		CString st = msg4 + msg3 + msg5;
		//GetLocalTime(&stNew);
	}

	return ret;
}


/*
	函数名称：InitialVideoSource
	输入参数：相机参数
			  实时视频流回调
	          实时解码图片回调
			  外部触发抓拍图片回调
	函数说明：初始化视频源，传入回调用于获取码流和外部触发抓拍的图片
*/
VIDEO_API handle __cdecl  InitialVideoSource(VideoParam *param, StreamCallback OnStreamReady, RGBCallback OnRealRGBReady, RGBCallback OnTrigRGBReady,void *pUserData)
{
	//初始化结构体指针
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
	//获取用户名及密码
	char bufferUserName[20] = { 0 };
	memset(bufferUserName, 0, 20);
	std::string strPath = GetModuleFilePath();
	
	//获取账户密码
	::GetPrivateProfileString("YTControl", "UserName", "admin", bufferUserPwd, MAX_PATH, strPath.c_str());
	::GetPrivateProfileString("YTControl", "PassWord", "admin", bufferUserName, MAX_PATH, strPath.c_str());
	int iStreamType = ::GetPrivateProfileInt("YTControl", "StreamType", 1, strPath.c_str()); //相机流类型 0:主码流 1:子码流

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
函数名称：InitialVideoSource3
输入参数：相机信息
		  实时视频流回调
		  解码数据输出回调
		  实时解码图片回调
		  外部触发抓拍图片回调
		  外部触发抓拍图片回调
函数说明：初始化视频源，传入回调用于获取码流和外部触发抓拍的图片
*/
VIDEO_API handle __cdecl  InitialVideoSource3(VideoParam *param, StreamCallback OnStreamReady, PixBufCallback OnPixBufReady, RGBCallback OnRealRGBReady, RGBCallback OnTrigRGBReady, TrigCallback OnTrigRGBReadyEx, void *pUserData)
{
	internal_handle *pih = NULL;
	char bufferUserPwd[20] = "";
	memset(bufferUserPwd, '\0', 20);
	//获取用户名及密码
	char bufferUserName[20] = { 0 };
	memset(bufferUserName, 0, 20);
	std::string strPath = GetModuleFilePath();
	//获取账户密码
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


/*获取视频源信息，返回0失败，1成功*/
VIDEO_API int   __cdecl  GetVideoInfo(handle hVS, int *decode, int *width, int *height, int *codec)
{
	internal_handle *pih = (internal_handle*)hVS;

	*decode = 0; //0-外部解码，1-内部解码
	*width = 0;
	*height = 0;
	*codec = AV_CODEC_ID_H264;

	return 1;
}

/*开启视频源，返回0失败，1成功*/
VIDEO_API int    __cdecl  StartVideoSource(handle hVS)
{
	internal_handle *pih = (internal_handle*)hVS;
	pih->bRealCallback = true;
	pih->bTrigCallback = true;
	return 1;
}

/*暂停视频源，调用该函数后仍然可以调用StartVideoSource开启*/
VIDEO_API void   __cdecl  StopVideoSource(handle hVS)
{
	internal_handle *pih = (internal_handle*)hVS;
	pih->bRealCallback = false;
	pih->bTrigCallback = false;
}

/*终止视频源模块*/
VIDEO_API void   __cdecl  FinializeVideoSource(handle hVS)
{
	internal_handle *pih = (internal_handle*)hVS;

	if (pih->hVideoChannel)
	{
		SNET_CloseChannel(pih->hVideoChannel);
		pih->hVideoChannel = NULL;
		Sleep(500);   //延迟500,等待子线程挂掉
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