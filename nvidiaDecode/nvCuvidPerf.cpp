/*
* 视频文件解析 H264 --->  RGB
*/

//系统头文件
#include <iostream>
#include <thread>
#include <iomanip>
#include <cassert>

//cuda头文件
#include <cuda_runtime.h>
#include <cuda.h>

//解码器头文件
#include "deepStream.h"
#include "dataProvider.h"
#include "ds_nvUtils.h"
#include "helper_cuda.h"

//使用Opencv实时显示
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

using namespace std;
using namespace cv;

#if 0
//输出数据结构体
typedef struct{
	int frameIndex;		//帧索引
	int videoIndex;		//视频索引
	int nWidthPixels;	//视频宽度
	int nHeightPixels;	//视频宽度
	uint8_t *dpFrame;	//帧数据
	size_t framesize;	//帧大小
	cudaStream_t stream;	//CUDA流
}DEC_OUTPUT;
#endif
//接收输出数据的回调函数
//typedef void (*StreamOutCallBack)(void *pUserData, DEC_OUTPUT *decOutput);

int g_devID 			= 0;			//GPUID
int g_nChannels 		= 0;			//多少路相机
int g_endlessLoop		= false;		//是否循环解码同一个视频文件	
char *g_fileList		= nullptr;		//文件列表
FILE *g_fp                      = nullptr;		//存储输出文件
cudaVideoCodec g_codec;

//打印相关信息
class DecodeProfiler : public IDecodeProfiler 
{
public:	
	void reportDecodeTime(const int frameIndex, const int channel, const int deviceId, double ms) 
	{
		nCount++;
		timeElasped += ms;
		if ((frameIndex+1) % interval == 0) 
		{
			LOG_DEBUG(logger, "Video [" << channel << "]:  " 
				<< std::fixed << std::setprecision(2) 
				<< "Decode Performance: " << float(interval * 1000.0 / timeElasped) << " frames/second"
				<< " || Decoded Frames: " << nCount);
			timeElasped = 0.;
		}
	}
	
	int nCount = 0;
	const int interval = 500;
	double timeElasped = 0.; // ms
};

//视频分析类，每一路解码器一个
std::vector<DecodeProfiler *> g_vpDecProfilers;

//源解码文件提供类
std::vector<FileDataProvider *> vpDataProviders;

//日志类
simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

//解析输入命令行
bool parseArg(int argc, char **argv);

//获取文件名
void getFileNames(const int nFiles, char *fileList, std::vector<std::string> &files);

//接收数据
void outputData(void *pData, DEC_OUTPUT *pOutData);


// user interface of inputi 提供数据
void userPushPacket(DataProvider *pDataProvider, IDeviceWorker *pDeviceWorker, const int channel);

int main(int argc, char **argv) {

	//打开文件准备写入文件
	g_fp = fopen("1.rgb", "wb");
	if(g_fp == NULL)
	{
		LOG_ERROR(logger, "打开文件失败 ");
		return -1;
	}

	//初始化1
        deepStreamInit();

	
	//解析输入参数1
	LOG_DEBUG(logger, "解析输入字符串" << argc);
	cout << "正在解析字符串" << endl;
	bool ret = parseArg(argc, argv);
	if (!ret) 
	{
		LOG_ERROR(logger, "Error in parseArg!");
		return 0;
	}
	
	// Init a worker on a GPU device ,负责workflow的所有工作 
	IDeviceWorker *pDeviceWorker = createDeviceWorker(g_nChannels, g_devID);
	
	// Add decode task 创建解码任务
	pDeviceWorker->addDecodeTask(g_codec);
	
	std::string strFiler = "aaa";

	//测试是否是转RGB合适的图片
//	pDeviceWorker->addColorSpaceConvertorTask(BGR_PLANAR);
	//
	
	//为woker提供profiers
#if 1
	for (int i = 0; i < g_nChannels; ++i) {
		g_vpDecProfilers.push_back(new DecodeProfiler);
		
		//先设置毁掉在启动解码
		pDeviceWorker->setDecCallback((void*)&strFiler, outputData, i);
		pDeviceWorker->setDecodeProfiler(g_vpDecProfilers[i], i);
	}
#endif
	
	
	// start the device worker.
	pDeviceWorker->start();	

	//设置取输出数据函数
//	pDeviceWorker->setDecCallback(NULL, outputData, 0);
	
	// User push video packets into a packet cachei
	std::vector<std::thread > vUserThreads;
	for (int i = 0; i < g_nChannels; ++i) 
	{
		vUserThreads.push_back( std::thread(userPushPacket, vpDataProviders[i], pDeviceWorker, i));	
	}
	
	// wait for user push threads
	for (auto& th : vUserThreads) {
		th.join();
	}
	
	// stop device worker
	pDeviceWorker->stop();
	
	if(g_fp != NULL)		
	{
		fclose(g_fp);	
		g_fp = NULL;	
	}
	// free
	while (!vpDataProviders.empty()) {
		FileDataProvider *temp = vpDataProviders.back();
		vpDataProviders.pop_back();
		delete temp;
	}
	
	pDeviceWorker->destroy();

	for (int i = 0; i < g_nChannels; ++i) {
		delete g_vpDecProfilers[i];
	}
	
	return 0;
}


//解析输入参数
bool parseArg(int argc, char **argv) {
	//返回值
	bool ret;
	
	int nDevs = 0;
	cudaError_t err = cudaGetDeviceCount(&nDevs);
	LOG_ERROR(logger, "nDevs " << nDevs);
	if (0 == nDevs) {
		LOG_ERROR(logger, "Warning: No CUDA capable device!");
		exit(1);
	}
	assert(err == cudaSuccess);

	g_devID = getCmdLineArgumentInt(argc, (const char **)argv, "devID");
	if (g_devID < 0 || g_devID >= nDevs) { 
		LOG_ERROR(logger, "Warning: No such GPU device!");
		return false; 
	}
	LOG_DEBUG(logger, "Device ID: " << g_devID);
	
	g_nChannels = getCmdLineArgumentInt(argc, (const char **)argv, "channels");
	if (g_nChannels <= 0) { return false; }
	LOG_DEBUG(logger, "Video channels: " << g_nChannels);
	
	g_endlessLoop = getCmdLineArgumentInt(argc, (const char **)argv, "endlessLoop");
	assert(0 == g_endlessLoop || 1 == g_endlessLoop);
	LOG_DEBUG(logger, "Endless Loop: " << g_endlessLoop);
	
	ret = getCmdLineArgumentString(argc, (const char **)argv, "fileList", &g_fileList);
	if (!ret) {
		LOG_ERROR(logger, "Warning: No h264 files.");
		return false;
	}
	
	std::vector<std::string > vFiles;
	getFileNames(g_nChannels, g_fileList, vFiles);
	g_codec = getVideoFormat(vFiles[0].c_str()).codec;
	LOG_DEBUG(logger, "g_codc " << g_codec);

	for (int i = 0; i < g_nChannels; ++i) {
		vpDataProviders.push_back(new FileDataProvider(vFiles[i].c_str()));
	}
	
	return true;
}

int g_nIndex = 0;

//解码出来的RGB视频帧是倒转的需要将数据进行反转
bool rgb32_rotate(int rate, uint8_t* srcRgb, int nWidth, int nHeight, bool direct)
{
	if(!srcRgb) return false;
	unsigned int n = 0;
	unsigned int lineSize = nWidth*4;
	uint8_t *destRgb = (uint8_t*)malloc(nWidth*nHeight*4);
	printf("%d--%d\n", nWidth, nHeight);
	if(direct)
	{
		for(int j=nWidth; j > 0; j--)
		{
			for(int i = 0; i < nHeight; i++)
			{
				memcpy(&destRgb[n], &srcRgb[lineSize*i+j*4-4], 4);
				n += 4;
				//printf("解码中1\n");
			}
		}
	}
	else
	{
		for(int j = 0; j < nWidth; j++)
		{
			for(int i = nHeight; i > 0; i--)
			{
				memcpy(&destRgb[n], &srcRgb[lineSize*(i-1)+j*4-4], 4);
				n += 4;
				//printf("解码中2\n");
			}
		}
	}
	memcpy(srcRgb, destRgb, nWidth*nHeight*4);
	free(destRgb);
	printf("反转结束\n");
	return true;
}


void outputData(void *pData, DEC_OUTPUT *pOutData)
{
	if(pOutData->dpFrame_ == NULL)
	  printf("输出YUV数据为空\n");

	//输出用户信息
	//std::string strFiler = *(std::string*)pData;
	//cout << strFiler.c_str() << endl;

	g_nIndex++;

	if(g_nIndex == 400)
	{
		LOG_DEBUG(logger, "开始写入数据"); 
		uint8_t *pBuf;
		pBuf = (uint8_t*)malloc(pOutData->frameSize_*4* sizeof(uint8_t));
		memset((char*)pBuf, 128, pOutData->frameSize_);
	//	cudaStream_t cst = pOutData->stream_;
	//	cudaMemcpyAsync(pBuf, pOutData->dpFrame_, pOutData->frameSize_, cudaMemcpyDeviceToHost, cst);	
	//	int len = fwrite(pBuf, 1, pOutData->frameSize_, g_fp);
	//	cout << len << endl;
	//	g_nIndex++;

		//free pBuf;
	
		//打印输出信息	
//		cout << buf << endl;
//	}
	//memcpy(buf, pOutData->dpFrame_, pOutData->frameSize_);

	//printf("%x", buf);

//	else
//	  printf("输出YUV数据不为空\n");	
#if 1
	//使用opencv将一阵数据保存为图片
//	if(g_nIndex == 500)
//	{
		//没有数据就崩溃
//		printf("%x", pOutData->dpFrame_[0]);
	
		//YUV420转码RGB
		//float *pRgb = new float[pOutData->frameSize_];		//申请空间存储RGB像素点
		//memset((char*)pRgb, 128, pOutData->frameSize_);
		
		uint8_t* devBuf;
//		cudaError_t cudaStatus = cudaHostMalloc((void**)&devBuf, pOutData->frameSize_, cudaHostAllocDefault);
		cudaError_t cudaStatus = cudaMalloc((void**)&devBuf, pOutData->frameSize_*4*sizeof(uint8_t));
		if(cudaStatus != cudaSuccess)
			printf("申请空间失败\n");

		cout << "开始转化文件" << endl;		

		//似乎转出来了，但是看不到图
		//nv12_to_bgr_planar_batch(pOutData->dpFrame_, 1.5*pOutData->nWidthPixels_,  (float*)devBuf, 3*pOutData->nWidthPixels_, pOutData->nWidthPixels_, pOutData->nHeightPixels_, 1, true, pOutData->stream_);
		
		//这个函数可以转出图廓来就是感觉跟蒙了一层纱一样
		nv12_to_bgra(pOutData->dpFrame_, pOutData->nWidthPixels_, devBuf, 4*pOutData->nWidthPixels_, pOutData->nWidthPixels_, pOutData->nHeightPixels_, pOutData->stream_);
		
		//转成灰度图片,也是灰灰的一片
		//nv12_to_gray_batch(pOutData->dpFrame_, 1.5*pOutData->nWidthPixels_, (float*)devBuf, 3*pOutData->nWidthPixels_, pOutData->nWidthPixels_, pOutData->nHeightPixels_, 1, pOutData->stream_);


		//LOG_DEBUG(logger, "输出内容RGB" << pRgb);
		//LOG_DEBUG(logger, "输出文件YUV大小-----------------------" << pOutData->frameSize_);
		
		cout << "转化文件完成" << endl;

		//cudaStream_t cst = pOutData->stream_;
		cudaMemcpyAsync(pBuf, devBuf, pOutData->frameSize_*4*sizeof(uint8_t), cudaMemcpyDeviceToHost, pOutData->stream_);	
		
//		cudaMemcpy(pBuf, devBuf, pOutData->frameSize_*2, cudaMemcpyDeviceToHost);	
		
		printf("%d--%d\n", pOutData->nWidthPixels_, pOutData->nHeightPixels_);
	
		//rgb32_rotate(90, pBuf, pOutData->nWidthPixels_, pOutData->nHeightPixels_, true);

		int len = fwrite(pBuf, 1, pOutData->frameSize_*4*sizeof(uint8_t), g_fp);
		cout << "写入文件大小" << len << endl;

		//fprintf(g_fp, "%d\n", pOutData->frameSize_);	
		//fprintf(g_fp, "%d\n", pOutData->nWidthPixels_);
		//fprintf(g_fp, "%d\n", pOutData->nHeightPixels_);	
	
		IplImage *pImg = cvCreateImage(cvSize(pOutData->nWidthPixels_, pOutData->nHeightPixels_), IPL_DEPTH_8U, 4);
		memcpy(pImg->imageData, pBuf, pOutData->nWidthPixels_*pOutData->nHeightPixels_* 4);
	//	cvNamedWindow("显示图片", 1);
	//	cvShowImage("显示图片", pImg);
		cvSaveImage("1.jpg", pImg);		
		
//		delete[] pRgb;
		cvReleaseImage(&pImg);		

		//g_nIndex++;
	}
#endif

//	g_nIndex++;

#if 0
	//else
	// printf("输出yuv数据不为空\n");
	//cout << "输出帧" << endl;
	if(g_fp != NULL)
	{		
		//申请空间存储RGB数据内存
		float *pRgb = new float[pOutData->frameSize_];

		memset(pRgb, '\0', pOutData->frameSize_);		

		//YUV转BGR平面格式
	int nv12_to_bgr_planar_batch(pOutData->dpFrame_, 1, pRgb, 1, pOutData->nWidthPixels_, pOutData->nHeightPixels_, pOutData->frameSize_, true, pOutData->stream_);
		
		LOG_DEBUG(logger, "输出文件内容："<< pRgb);
		//写入文件
		fwrite(pRgb, 1, pOutData->frameSize_, g_fp);
		
		delete[] pRgb;
#if 0
		cout << "开始写入文件" << endl;
		cout << pOutData->frameSize_ << endl;
		cout << pOutData->nWidthPixels_ << endl;
		cout << pOutData->nHeightPixels_ << endl;
#endif
		//printf("%d_%d_%d\n", pOutData->dpFrame_[0], pOutData->dpFrame_[1], pOutData->dpFrame_[2]);
		//printf("%s", pOutData->dpFrame_);
		//uleep(10);
#if 0
		if(g_nIndex == 0)
		{		
			//int len = fwrite((const void*)pOutData->dpFrame_, pOutData->frameSize_, 1, g_fp);
			//cout << "写入文件的大小--------  " << len << endl;
			#if 0
			char * ag;
			ag = (char*)malloc(pOutData->frameSize_);
			memcpy(ag, pOutData->dpFrame_, pOutData->frameSize_);
			cout << "拷贝成功" << endl;
			int len = fwrite(ag, 1, pOutData->frameSize_, g_fp);
			#endif

			float *ag = new float[1 << 20];  //1M	
		//	ag = (uint8_t*)malloc(1000);
			memcpy(ag, pRgb, 1 << 20);
			cout << "拷贝成功" << endl;
			int len = fwrite(pRgb, 1, 1 << 20, g_fp);
			cout << "写入文件大小--------- " << len << endl;
			g_nIndex++;
			delete[] ag;
			fclose(g_fp);		//关闭文件指针
			g_fp = NULL;
		}	
#endif
	}
#endif 
}


void getFileNames(const int nFiles, char *fileList, std::vector<std::string> &files) 
{
	int count = 0;
	char *str;
	str = strtok(fileList, ",");
	while (NULL != str) 
	{
		files.push_back(std::string(str));
		str = strtok(NULL, ",");
		count++;
		if (count >= nFiles) {
			break;
		}
	}
}

//接收输出数据


void userPushPacket(DataProvider *pDataProvider, IDeviceWorker *pDeviceWorker, const int channel) 
{
	assert(NULL != pDeviceWorker);
	assert(NULL != pDataProvider);
	int nBuf = 0;
	uint8_t *pBuf = nullptr;
	int nPkts = 0;
	
	while (true) 
	{
		// get a frame packet from a video file
		int bStatus = pDataProvider->getData(&pBuf, &nBuf);
		if (bStatus == 0) 
		{
			if (g_endlessLoop) 
			{
				//LOG_DEBUG(logger, "User: Reloading...");
				pDataProvider->reload();
			} 
			else 
			{
				LOG_DEBUG(logger, "User: Ending...");
				// push the last NAL unit packet into deviceWorker
				pDeviceWorker->pushPacket(pBuf, nBuf, channel);
				pDeviceWorker->stopPushPacket(channel);
				break;
			}
		}
		 else 
		{
			// Push packet into deviceWorker.
			pDeviceWorker->pushPacket(pBuf, nBuf, channel);
	//		cout << "输入数据大小：--"<< nBuf << endl;
	//		cout << "数据内容： " << pBuf << endl;
	//		sleep(1000);
		}
	}
}

