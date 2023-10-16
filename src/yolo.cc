#include<unistd.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<fstream>
#include<NvInferPlugin.h>
#include<cuda_runtime_api.h>
#include<opencv2/imgproc.hpp>
#include<opencv2/highgui.hpp>
#include<map>
#include<cmath>
#include<json/json.h>
#include"cuda_utils.h"
#include"preprocess.h"
#include"yolo.hh"
#include<pthread.h>
#define DEVNUMS 13 

extern pthread_mutex_t mutex1;
unsigned int detectRecNums=0;
extern DetectInfo detectInfos[60*20];
extern vector<ImgDevID> imgDevIDs;
extern pthread_mutex_t ImgLocks[DEVNUMS];

extern pthread_mutex_t mutex2;
extern pthread_cond_t cond2;
extern int thread_status[DEVNUMS];
using namespace std;
using namespace Json;

YoloNas::YoloNas(const string& plan_path,unsigned int max_batch_size,const char* filename) 
	:MAX_BATCH_SIZE(max_batch_size)
{
	ifstream ifs("imgParam.json");	
	if(!ifs.is_open())
	{
		cout<<"open imgParam.json file err"<<endl;
		exit(1);
	}
	
	Reader reader;
	Value root;
	bool bl=reader.parse(ifs,root);
	if(!bl)
	{
		cout<<"reader.parse err"<<endl;
		exit(1);

	}

	ifs.close();
	
	INPUT_H=root["INPUT_H"].asUInt();
	INPUT_W=root["INPUT_W"].asUInt();

	RAW_IMG_H=root["RAW_IMG_H"].asUInt();
	RAW_IMG_W=root["RAW_IMG_W"].asUInt();

	CONF_THR=root["CONF_THR"].asFloat();
	IOU_THR=root["IOU_THR"].asFloat();

	MAX_IMAGE_INPUT_SIZE_THRESH=root["MAX_IMAGE_INPUT_SIZE_THRESH"].asUInt();

	MAX_COL_BLOCKS=INPUT_H*INPUT_W/1024 + INPUT_H*INPUT_W/256 + INPUT_H*INPUT_W/64;

	buffer_size_[0]=MAX_BATCH_SIZE*3 * INPUT_H*INPUT_W;		//存放一次批处理的图像总像素数
	buffer_size_[1]=MAX_BATCH_SIZE*MAX_COL_BLOCKS*4;		//存放一次批处理的所有列块数的四个角
	buffer_size_[2]=MAX_BATCH_SIZE*MAX_COL_BLOCKS;		//存放一次批处理图像所有列块数的置信度
	buffer_size_[3]=MAX_BATCH_SIZE*MAX_COL_BLOCKS;		//存放一次批处理图像所有列块数的类别

	cudaMalloc(&buffers_[0],buffer_size_[0]*sizeof(float));		//在GPU上分配内存空间，起始地址赋给buffers_[0]
	cudaMalloc(&buffers_[1],buffer_size_[1]*sizeof(float));
	cudaMalloc(&buffers_[2],buffer_size_[2]*sizeof(float));
	cudaMalloc(&buffers_[3],buffer_size_[3]*sizeof(float));

	boxes_.resize(buffer_size_[1]);		//给数组容器分配大小
	scores_.resize(buffer_size_[2]);
	classes_.resize(buffer_size_[3]);

	radtio_w=(RAW_IMG_W*1.0)/(INPUT_W*1.0);
	radtio_h=(RAW_IMG_H*1.0)/(INPUT_H*1.0);
	
	std::cout<<"radtio_w:"<<radtio_w<<"  radtio_h:"<<radtio_h<<"  RAW_IMG_W:"<<RAW_IMG_W<<"  RAW_IMG_H:"<<RAW_IMG_H<<std::endl;

	//prepare input data cache in pinned memory 
	cudaMallocHost((void**)&img_host,MAX_IMAGE_INPUT_SIZE_THRESH*3);	//分配在主机上分配固定内存
	//prepare input data cache in device memory
	cudaMalloc((void**)&img_device,MAX_IMAGE_INPUT_SIZE_THRESH*3);		//在GPU上分配内存

	detections.resize(MAX_BATCH_SIZE);	//每张图片对应一个数组容器，用来存放推理结果信息

	cudaStreamCreate(&stream_);
	LoadEngine(plan_path);
}


void YoloNas::LoadEngine(const std::string& engine_file) 
{
	std::ifstream in_file(engine_file,std::ios::binary);	//二进制方式打开引擎文件
	if (!in_file.is_open()) 
	{
		std::cerr<<"Failed to open engine file: "<<engine_file<< std::endl;
		return;
	}

	in_file.seekg(0,in_file.end);
	int length=in_file.tellg();
	in_file.seekg(0,in_file.beg);
	std::unique_ptr<char[]> trt_model_stream(new char[length]);
	in_file.read(trt_model_stream.get(),length);	//文件读到独占的智能指针中
	in_file.close();

	initLibNvInferPlugins(&g_logger_, "");	//初始化日志记录器,并使用默认的插件库

	runtime=nvinfer1::createInferRuntime(g_logger_);	//创建运行时对象
	assert(runtime != nullptr);

	engine_=runtime->deserializeCudaEngine(trt_model_stream.get(),length,nullptr);	//创建引擎对象
	assert(engine_ != nullptr);

	context_=engine_->createExecutionContext(); 	//创建推理上下文
	assert(context_ != nullptr);

	auto input_dims=nvinfer1::Dims4{MAX_BATCH_SIZE,3,INPUT_H,INPUT_W};	//Dims4表示具有4个维度的张量
	//0表示第一个输入张量,input_dims表示这个张量的维度
	context_->setBindingDimensions(0,input_dims);	//绑定索引为0的输入张量的维度为input_dims
}


void YoloNas::detection() 
{
	//Preprocessing
	size_t size_image_dst=INPUT_H*INPUT_W*3;	//单张目标图片总像素数
	size_t size_image=RAW_IMG_W*RAW_IMG_H*3;	//单张原始图片总像素数
	float* buffer_idx=(float*)buffers_[0];

	//图像进入GPU缓存   
	for (int i=0;i<MAX_BATCH_SIZE;i++)
	{    
		pthread_mutex_lock(&ImgLocks[i]);
		memcpy(img_host,imgDevIDs[i].img.data,size_image);	//原始图像数据进入主机内存
		pthread_mutex_unlock(&ImgLocks[i]);

		//原始图像数据进入设备内存
		cudaMemcpyAsync(img_device,img_host,size_image,cudaMemcpyHostToDevice,stream_);		
		cudaStreamSynchronize(stream_);

		//把设备内存中的原始图片压缩后，放入buffer_idx内存地址中
		preprocess_kernel_img(img_device,RAW_IMG_W,RAW_IMG_H,buffer_idx,INPUT_W,INPUT_H,stream_);			
		cudaStreamSynchronize(stream_);

		buffer_idx+=size_image_dst;
	}
	
	//等待8个子线程都截图完成
	for(int i=0;i<DEVNUMS;i++)    
	{   
		while(thread_status[i]==0)
		{   
			//空循环
		}   
	} 

	
/*
	//等待8个子线程都截图完成
	for(int i=0;i<DEVNUMS;i++)    
	{   
		while(thread_status[i]==0)
		{   
			//空循环
		}   
	} 

	auto end1 = std::chrono::system_clock::now();
    std::cout << "all screenshot time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count() << "ms" << std::endl;

*/

	auto start = std::chrono::system_clock::now();


	//Do inference
	//context_->enqueue(1, &buffers_[0], stream_, nullptr);	//这个函数执行完，buffers_[1-3]的数据会自动被填充
	context_->enqueueV2(buffers_,stream_,nullptr);
	cudaStreamSynchronize(stream_);

	cudaMemcpyAsync(boxes_.data(),buffers_[1],buffer_size_[1]*sizeof(float),cudaMemcpyDeviceToHost,stream_);
	cudaStreamSynchronize(stream_);

	cudaMemcpyAsync(scores_.data(),buffers_[2],buffer_size_[2]*sizeof(float),cudaMemcpyDeviceToHost,stream_);
	cudaStreamSynchronize(stream_);

	cudaMemcpyAsync(classes_.data(),buffers_[3],buffer_size_[3]*sizeof(float),cudaMemcpyDeviceToHost,stream_);
	cudaStreamSynchronize(stream_);

	//confidence过滤
	vector<DetectInfo> tmp_detectInfos;
	for(int a=0;a<MAX_BATCH_SIZE;++a)
	{
		auto& res=detections[a];	//detections[a]是一个数组容器,类型为vector<Detection>,即数组的每个元素都是Detection类型	
		res.clear();	//清空res数组容器
		nms(res,a,CONF_THR,IOU_THR);	//每次处理一张图片，处理的结果会放入res数组容器中
	
		std::cout<<"devID:"<<imgDevIDs[a].devID<<",detection size="<<detections[a].size()<<endl;	

		//画框
		for(auto det:detections[a])	//依次取出detections[a]容器数组中的每个元素
		{
	
			if(det.classes != 0) continue;

            int x2=(int)((det.box[0]*65535)/INPUT_W);
            int y2=(int)((det.box[1]*65535)/INPUT_H);
            int w=(int)(((det.box[2]-det.box[0])*65535)/INPUT_W);
            int h=(int)(((det.box[3]-det.box[1])*65535)/INPUT_H);
    
            int x1=x2+0.5*w;
            int y1=y2+0.5*h;
    
    
            int x3=det.box[0]*radtio_w; //左上角的x坐标
            int y3=det.box[1]*radtio_h; //左上角的y坐标
            int w3=(det.box[2]-det.box[0])*radtio_w;
            int h3=(det.box[3]-det.box[1])*radtio_h;		

			cv::rectangle(imgDevIDs[a].img,
						  cv::Rect(x3,y3,w3,h3),	//矩形的x1,y1,w,h
						  cv::Scalar(0x27, 0xC1, 0x36),	//rgb颜色
						  2);	//线宽

			//先写入其他内存中，后面再同一写入共享内存
			DetectInfo tmp_detecInfo{}; 	
			strcpy( tmp_detecInfo.devID,imgDevIDs[a].devID.c_str() );
			tmp_detecInfo.x=x1;
			tmp_detecInfo.y=y1;
			tmp_detecInfo.w=w;
			tmp_detecInfo.h=h;
			
			tmp_detectInfos.push_back(tmp_detecInfo);
		}
		if(a==2)
		{
			cv::imshow("yolo",imgDevIDs[a].img);
			cv::waitKey(5);
		}

	}

	//加锁
	pthread_mutex_lock(&mutex1);	//对全局结构体数组操作之前先加锁

	memset(detectInfos,0,sizeof(detectInfos));	//清空全局结构体数

	//写入全局结构体数组
	for(int i=0;i<tmp_detectInfos.size();i++)
	{
		//cout<<"before memshm"<<endl;
		strcpy(detectInfos[i].devID,tmp_detectInfos[i].devID);
		//cout<<"detectInfos["<<i<<"].devID"<<detectInfos[i].devID<<endl;
		detectInfos[i].x=tmp_detectInfos[i].x;		
		//cout<<"detectInfos["<<i<<"].x"<<detectInfos[i].x<<endl;
		detectInfos[i].y=tmp_detectInfos[i].y;		
		//cout<<"detectInfos["<<i<<"].y"<<detectInfos[i].y<<endl;
		detectInfos[i].w=tmp_detectInfos[i].w;
		//cout<<"detectInfos["<<i<<"].w"<<detectInfos[i].w<<endl;
		detectInfos[i].h=tmp_detectInfos[i].h;
		//cout<<"detectInfos["<<i<<"].h"<<detectInfos[i].h<<endl;
	}

	detectRecNums=tmp_detectInfos.size();	//对全局变量的操作也是要加锁
	pthread_mutex_unlock(&mutex1);	//解锁

	auto end = std::chrono::system_clock::now();
    std::cout << "inference time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
pthread_mutex_lock(&mutex2);    
	for (int i = 0; i <DEVNUMS ; i++) 
	{   
		thread_status[i] = 0;
	}
	pthread_mutex_unlock(&mutex2);
	auto start1 = std::chrono::system_clock::now();
    pthread_cond_broadcast(&cond2);


}




YoloNas::~YoloNas() 
{
	cudaStreamDestroy(stream_);
	for (auto& buffer : buffers_) 
	{
		cudaFree(buffer);
	}

	if (context_ != nullptr) 
	{
		runtime->destroy();
		context_->destroy();
		engine_->destroy();
	}
}

float iou(float lbox[4], float rbox[4]) {
	float interBox[] = {
		(std::max)(lbox[0] - lbox[2] / 2.f , rbox[0] - rbox[2] / 2.f), //left
		(std::min)(lbox[0] + lbox[2] / 2.f , rbox[0] + rbox[2] / 2.f), //right
		(std::max)(lbox[1] - lbox[3] / 2.f , rbox[1] - rbox[3] / 2.f), //top
		(std::min)(lbox[1] + lbox[3] / 2.f , rbox[1] + rbox[3] / 2.f), //bottom
	};

	if (interBox[2] > interBox[3] || interBox[0] > interBox[1])
		return 0.0f;

	float interBoxS = (interBox[1] - interBox[0])*(interBox[3] - interBox[2]);
	return interBoxS / (lbox[2] * lbox[3] + rbox[2] * rbox[3] - interBoxS);
}

bool cmp(const Detection& a, const Detection& b) {
	return a.score > b.score;
}

void YoloNas::nms(std::vector<Detection>& res,int batch,float conf_thresh,float nms_thresh) 
{
	std::map<float,std::vector<Detection>> m;
	for (int i=0;i<MAX_COL_BLOCKS;i++) 
	{
		if(scores_[batch*MAX_COL_BLOCKS+i]<=conf_thresh) continue;

		Detection det;
		det.score=scores_[batch*MAX_COL_BLOCKS+i];
		det.classes=classes_[batch*MAX_COL_BLOCKS+i];       
		memcpy(&det.box[0],&boxes_[batch*MAX_COL_BLOCKS*4+i*4],4*sizeof(float));
		m[det.classes].push_back(det);
	}

	for (auto it = m.begin(); it != m.end(); it++) 
	{
		//std::cout << it->second[0].class_id << " --- " << std::endl;
		auto& dets = it->second;
		std::sort(dets.begin(), dets.end(), cmp);
		for (size_t m = 0; m < dets.size(); ++m) {
			auto& item = dets[m];
			res.push_back(item);
			for (size_t n = m + 1; n < dets.size(); ++n) {
				if (iou(item.box, dets[n].box) > nms_thresh) {
					dets.erase(dets.begin() + n);
					--n;
				}
			}
		}
	}
}
