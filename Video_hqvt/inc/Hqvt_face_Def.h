#ifndef _HQVT_FACE_H	
#define _HQVT_FACE_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------*/
//人脸识别
/*------------------------------------------------------------------------------*/
typedef struct _FACE_DETECT_PARAM_S
{
	DWORD				dwSize;
	BYTE				bEnable;		//是否开启智能人脸抓拍功能
	BYTE				bPicDispFaceArea;//抓怕照片是否叠加人脸框
	BYTE				bSnapMode;		//抓怕模式，0x1:人离开后抓怕,0x2:实时抓怕,0x4:间隔抓拍
	BYTE				bSnapThreshold;	//抓怕阀值，range 0-10,default 1，(5.0.36之后版本弃用)
	AREA_RECT_S			stDetectArea;	//智能检测区域 坐标参考704*576
	float				fSensitivity;	//检测灵敏度 rang 0-1,default 0.4
	BYTE                bPreviewFaceArea;//预览是否叠加人脸框 0 不叠加 1 叠加人脸及头肩 2 仅叠加人脸，不叠加头肩
	BYTE                bShootPicType;  //抓拍上传 0x1上传原图 0x2 上传人脸图片 0x3上传原图和人脸抠图
	BYTE				bPicSendSvr;    //图片上传至人脸服务器
	BYTE				bPicSendFtp;    //图片上传至FTP	
	URL_PORT_S          stFaceSvr;      //人脸图片服务器地址
	BYTE				bFaceAreaAE;	//是否开启人脸区域自动曝光 0 普通模式 1 自动模式
	BYTE				bFaceROI;		//是否开启人脸感兴趣区域编码
	WORD                wFaceMinPixel;  //人脸检测最小像素 32~672，默认值96对应:(720p default: 64*64 max: 450*450)
	DWORD               dwRealShootTrackFr;//实时抓拍最长跟踪帧数
	float               fPeriodShootIntv;//间隔抓拍时间，单位秒
	WORD                wMaxSnapPicCnt;  //最大抓拍数: 限制每个人脸目标最大抓拍图片数量,0不限制
	BYTE                bMaxFaceCntLimit;//单帧最大人脸数,0不限制    
	BYTE                bFaceCreateDist; //人脸生成移动距离 1-30，值越大人脸生成速度越慢
	WORD				wSnapFaceWidth;	 //人脸抓图大小,0保持原始宽度
	WORD				wSnapFaceHeight; //人脸抓图大小,0保持原始高度
	SEG_TIME_S          stSegTime[2];    //人脸检测时间段
	//--------------------------------------------------------------------------------------------------------------
	BYTE                bySendFtpPathType;//上传FTP路径 0 以设备ID为主目录 1 以当前日期为主目录
	BYTE                byUpgradeMode;  //升级方式 0: ZMODEM 1：STREAM
	BYTE                byFacePicQulity;//人脸图抓拍图片编码质量
	BYTE                byConnCenter;   // 接入人脸管理平台
	URL_PORT_S          stCenterAddr;   // 接入服务器IP端口
	char                szConnectID[MAX_ID_LEN];//接入ID
	float               fFacePicSceneRatio; //人脸图片周边区域系数 
	BYTE				byFaceExpAutoModeStrength;//人脸曝光自适应模式强度值:0~30, 目前已弃用(5.0.25之后版本弃用)
	BYTE				byFaceBright;		//人脸亮度	:0~30, 目前已弃用(5.0.25之后版本弃用)
	BYTE                byRes1[2];
	DWORD               dwUpgradeBaudrate;  //升级波特率: 115200/230400/460800/576000/921600/1000000
	BYTE				byRes2[14];
	BYTE                bfaceAlgoMonitor;//是否启用算法监控 0 不启用 1 监控算法是否运行 2 监控算法是否正常检测人脸
	BYTE                bProcSnapMode;   //抓图处理方式 0: 异步 1: 同步(调试参数，Release版本将使用异步方式，弃用此参数?) 同步方式:直接在接收数据线程处理人脸曝光/人脸ROI/人脸抓图...
										 //异步方式: 在接收数据线程仅处理人脸曝光/人脸ROI，新建线程处理人脸抓图，防止因为人脸抓图处理延时导致人脸曝光/人脸ROI/结果推送后端处理不及时
	BYTE                bySendSvrProtocol;//人脸图片上传服务器上传方式 0: 自定义协议上传 1：HTTP上传
	BYTE                byRes3[3];  //
	DWORD               dwPicHttpPostId;
	char                szPicHttpPostPath[100];
	BYTE				bStartSnapQulity;   //人脸抓怕质量阀值，范围0-100，当分析出来的人脸质量>=设置的抓怕质量阀值时开始抓拍
	BYTE                byRes4[627];
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	BYTE                bySeparate;
	BYTE                byEnableBodySnap;//仅人体抓拍特殊版本有效
	WORD                wBodyPicSceneRatioTop;//仅人体抓拍特殊版本有效
	WORD                wBodyPicSceneRatioBotton;//仅人体抓拍特殊版本有效              
	WORD                wBodyPicSceneRatioLeft;//仅人体抓拍特殊版本有效
	WORD                wBodyPicSceneRatioRight;//仅人体抓拍特殊版本有效
	WORD                wBodyPicSnapTrackFr;//仅人体抓拍特殊版本有效
	BYTE				byRes5[256];
}FACE_DETECT_PARAM_S;

/*
typedef struct _FACE_DETECT_PARAM_S
{
	DWORD				dwSize;
	BYTE				bEnable;		//是否开启智能人脸抓拍功能
	BYTE				bPicDispFaceArea;//抓怕照片是否叠加人脸框
	BYTE				bSnapMode;		//抓怕模式，0x1:人离开后抓怕,0x2:实时抓怕,0x4:间隔抓拍
	BYTE				bSnapThreshold;	//抓怕阀值，range 1-10,default 4
	AREA_RECT_S			stDetectArea;	//智能检测区域 坐标参考704*576
	float				fSensitivity;	//检测灵敏度 rang 0-1,default 0.4
	BYTE                bPreviewFaceArea;//预览是否叠加人脸框 0 不叠加 1 叠加人脸及头肩 2 仅叠加人脸，不叠加头肩
	BYTE                bShootPicType;  //抓拍上传 0x1上传原图 0x2 上传人脸图片 0x3上传原图和人脸抠图
	BYTE				bPicSendSvr;    //图片上传至人脸服务器
	BYTE				bPicSendFtp;    //图片上传至FTP	
	URL_PORT_S          stFaceSvr;      //人脸服务器地址
	BYTE				bFaceAreaAE;	//是否开启人脸区域自动曝光
	BYTE				bFaceROI;		//是否开启人脸感兴趣区域编码
	WORD                wFaceMinPixel;  //人脸检测最小像素 32~672，默认值96对应:(720p default: 64*64 max: 450*450)
	DWORD               dwRealShootTrackFr;//实时抓拍最长跟踪帧数
	float               fPeriodShootIntv;//间隔抓拍时间，单位秒
	WORD                wMaxSnapPicCnt;  //最大抓拍数: 限制每个人脸目标最大抓拍图片数量,0不限制
	BYTE                bMaxFaceCntLimit;//单帧最大人脸数,0不限制    
	BYTE                bFaceCreateDist; //人脸生成速度 1-5
	WORD				wSnapFaceWidth;	 //人脸抓图大小,0保持原始宽度
	WORD				wSnapFaceHeight; //人脸抓图大小,0保持原始高度
	SEG_TIME_S          stSegTime[2];
	//--------------------------------------------------------------------------------------------------------------
	BYTE                bySendFtpPathType;
	BYTE                byRes1[2];
	BYTE                byConnCenter;   // 接入人脸管理平台
	URL_PORT_S          stCenterAddr;   // 接入服务器IP端口
	char                szConnDeviceID[32];//接入ID
	float               fFacePicSceneRatio;
	BYTE				byFaceExpAutoModeStrength;	 //人脸曝光自适应模式强度值
	BYTE				byFaceBright;		//人脸亮度		
	BYTE				byRes2[20];
	BYTE                bfaceAlgoMonitor;//是否启用算法监控
	BYTE                bProcSnapMode;   //抓图处理方式 0: 异步 1: 同步(调试参数，Release版本将使用异步方式，弃用此参数?) 同步方式:直接在接收数据线程处理人脸曝光/人脸ROI/人脸抓图...
	//异步方式: 在接收数据线程仅处理人脸曝光/人脸ROI，新建线程处理人脸抓图，防止因为人脸抓图处理延时导致人脸曝光/人脸ROI/结果推送后端处理不及时
}FACE_DETECT_PARAM_S;
*/
// typedef struct __ICE_FACE_RESULT_S
// {
//     DWORD       dwSize;						//上传的数据总长度，目前为该结构体长度
// 	DWORD		dwTimeStamp;				//时间戳信息,用来和视频帧同步，暂不同步看下效果
// 	INT			dwAlgoWidth;				//dwAlgoWidth和dwAlgoHeight是算法处理的图像宽和高
// 	INT			dwAlgoHeight;				//上报的人脸坐标参考坐标系即为(dwAlgoWidth, dwAlgoHeight)
// 	INT 		s32TargetNum;				//!< 人脸数目
// 	AREA_RECT_S astTargets[32];				//!< 人脸坐标
// 
// 	BYTE		byRes[236];				
// }ICE_FACE_RESULT_S; 

typedef struct __ICE_FACE_RESULT_S
{
	DWORD       dwSize;         // 上传的数据总长度，目前为该结构体长度
	DWORD		dwTimeStamp;    // 时间戳信息,用来和视频帧同步，暂不同步看下效果
	INT			dwAlgoWidth;	//dwAlgoWidth和dwAlgoHeight是算法处理的图像宽和高
	INT			dwAlgoHeight;   //上报的人脸坐标参考坐标系即为(dwAlgoWidth, dwAlgoHeight)
	INT 		s32TargetNum;   //目标数目，包括人脸或头肩
	AREA_RECT_S astTargetArea[32]; //目标位置
	BYTE        byTargetType[32];  //目标类型 0: 人脸 1: 头肩
	DWORD       dwTargetId[32];    //目标ID，跟踪号
	BYTE        byTargetQuality[32];//目标人脸质量评分
	BYTE		byRes[44];
}ICE_FACE_RESULT_S; 	


typedef struct _ICEFACE_MSG_NOTIFY
{
	HANDLE					hLogonServer;
	DWORD					dwClientID;
	DWORD					dwServerIP;
	DWORD					dwServerPort;
	ICE_FACE_RESULT_S		stIceFaceResult;
}ICEFACE_MSG_NOTIFY;

//typedef struct _FACE_PIC_INFO_S
//{
//	DWORD				dwBgnFlag;			//0x12345678
//	DWORD				dwSize;				//结构大小 sizeof(FACE_PIC_INFO_S)	
//	DWORD				dwIPAddr;			//设备IP
//	DWORD				dwDeviceID;			//设备ID
//	long long			u64Time;			//抓图的时间    
//	WORD				wImageWidth;		//图片宽度
//	WORD				wImageHeight;		//图片高度	
//	DWORD   			dwPicLen;
//	FACE_AREA_S			stTargets;			//人脸坐标
//	DWORD				stTrackID;			//跟踪号
//	DWORD 				dwPicType;			//图片类型	0 人离开后抓拍 1 实时抓拍
//	char				dwReserved[204];	//备用2		
//	DWORD				dwEndFlag;			//0x87654321
//}FACE_PIC_INFO_S; 

typedef struct _FACE_FEATURE_INFO
{
	DWORD   			dwDataLen;
}FACE_FEATURE_INFO;/*人脸特征信息*/

typedef struct __FACE_AREA_S
{
	WORD				wRoiLeft;
	WORD				wRoiTop;
	WORD				wRoiRight;
	WORD				wRoiBottom;
}FACE_AREA_S;

typedef struct _FACE_SNAP_PIC_INFO
{
	DWORD              dwPicLen;           //图片长度
	UInt8               byCapType;          //抓拍类型	0x1 人离开后抓拍 0x2 实时抓拍 0x4 间隔抓拍
	UInt8               byImageType;        //图片类型	0x1 场景图 0x2 人脸图 0x4 人体图
	UInt8               byQulity;           //0~100
	UInt8               byEncoderType;      //目前，值为0代表jpeg图片
	long long           u64CapTime;         //抓图的时间    
	WORD              wImageWidth;        //图片宽度
	WORD              wImageHeight;       //图片高度  
	FACE_AREA_S         stTgtFaceArea;      //人脸坐标
	WORD                wCapMsec;           //抓图的时间，毫秒
	UInt8               byRes[18];
}FACE_SNAP_PIC_INFO;/*图片信息*/


typedef struct _MYIP6ADDR
{
	UInt8		u6_addr8[16];
}IP6ADDR;

typedef struct _FACE_PICS_INFO_S
{
	DWORD              dwBgnFlag;          //0x12345678
	DWORD              dwSize;             //消息长度 = wHeadSize + dwTotDataLen;为一次图片传输事务发起的消息通讯包的总长度
	DWORD              dwIPAddr;           //设备IP,当byIpAddrType为0时有效
	DWORD              dwDeviceID;         //设备ID
	DWORD              dwTrackID;          //人脸跟踪号
	WORD               wVersion;           //协议版本号，目前为1
	UInt8              byIpAddrType;       //摄像机IP地址类型: 0: ipv4 1:ipv6
	UInt8               byRes1[1];
	WORD                wHeadSize;          //结构大小 sizeof(FACE_PICS_INFO_S)
	UInt8               byFeatureNum;       //人脸特征数据个数，目前不支持，值为0
	UInt8               byPicNum;           //图片数
	DWORD              dwTotDataLen;       //图片裸数据+人脸特征数据总长度，目前暂不支持人脸特征数据，为图片裸数据总长度
	FACE_SNAP_PIC_INFO  stPics[2];          //图片信息
	FACE_FEATURE_INFO   stFeature[2];       //人脸特征数据信息
	char				byRes2[84];	        //备用2	
	DWORD				dwDevSendPicTimes;  //摄像机上传图片计数，设备启动后，第一次上传图片为0，每次上传图片计数加1
	IP6ADDR             stIpAddr6;          //摄像机ipv6地址，当byIpAddrType为1时有效，按struct in6_addr结构解析
	long long           u64TgtCreateTime;   //人脸目标生成时间
    DWORD				dwTgtSendPicTimes;  //当前抓拍目标上传图片计数，目标第一次上传图片为0，该目标每次上传图片，计数加1
	DWORD              dwEndFlag;          //0x87654321
}FACE_PICS_INFO_S;


#ifdef __cplusplus
}
#endif

#endif

