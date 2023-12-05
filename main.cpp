#include"main.h"

#define DEVNUMS 2

extern unsigned int detectRecNums;	//在yolo.cc中定义	
extern vector<MidDevID> midDevIDs;	//在hcSDK.cpp中定义

pthread_mutex_t mutex1=PTHREAD_MUTEX_INITIALIZER;	//在数据段上定义并初始化互斥锁，程序结束自动释放，无需手动释放

pthread_mutex_t imgLocks[DEVNUMS];	//在数据段上定义互斥锁数组，没有初始化

pthread_mutex_t mutex2=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2=PTHREAD_COND_INITIALIZER;

vector<ImgDevID> imgDevIDs;

DetectInfo detectInfos[60*20];

map<LONG,int> portToIndex;

int thread_status[DEVNUMS]={0};

int sys_err(const char* str)
{
	perror(str);

	exit(1);
}

void* handler(void* arg)	//服务器处理线程
{
	sleep(10);
	hcSDK* hcsdk=static_cast<hcSDK*>(arg);

	UdpComm udpComm;	//创建一个udpComm对象用于和客户端通信，创建套接字和绑定服务区地址结构

   	while(1)
    {
		if(udpComm.recvReq()!=0)
		{
			continue;
		}
		
        //根据接收到的mid，获取对应的2个devID
		udpComm.getTwoDevID(hcsdk->m_allDevNums);

		std::cout<<"udpComm.devID[0]="<<udpComm.devID[0]<<std::endl;
		std::cout<<"udpComm.devID[1]="<<udpComm.devID[1]<<std::endl;

		//加锁
		pthread_mutex_lock(&mutex1);

		//遍历全局变量，获取报警情况
		udpComm.getAlmStat(detectInfos);

		std::cout<<"num1="<<udpComm.num[0]<<std::endl;
		std::cout<<"num2="<<udpComm.num[1]<<std::endl;

		//遍历共享内存，将设备报警信息存在结构体数组中
		const int num1=udpComm.num[0];
		const int num2=udpComm.num[1];

		 //对应devID[0]有告警，devID[1]无告警的情况
		if(num1!=0 && num2==0)
		{
			DetectInfo detecAlarm1[num1];
			udpComm.saveAlmInfo1(detecAlarm1,num1,detectInfos);
			pthread_mutex_unlock(&mutex1);	//解锁
			
			udpComm.writeXML1(detecAlarm1);
		}	
		//对应devID[0]无告警，devID[1]有告警的情况
		else if(num1==0 && num2!=0)
		{
			DetectInfo detecAlarm2[num2];
			udpComm.saveAlmInfo2(detecAlarm2,num2,detectInfos);
			pthread_mutex_unlock(&mutex1);	//解锁
			
			udpComm.writeXML2(detecAlarm2);
		}
		//对应devID[0]和devID[1]都有告警的情况
		else if(num1!=0 && num2!=0)
		{
			DetectInfo detecAlarm1[num1];
			udpComm.saveAlmInfo1(detecAlarm1,num1,detectInfos);

			DetectInfo detecAlarm2[num2];
			udpComm.saveAlmInfo2(detecAlarm2,num2,detectInfos);
			pthread_mutex_unlock(&mutex1);	//解锁
			

			udpComm.writeXML3(detecAlarm1,detecAlarm2);
		}
		//对应devID[0]和devID[1]都没有告警的情况
		else if(num1==0 && num2==0)
		{
			pthread_mutex_unlock(&mutex1);	//解锁
			udpComm.writeXML4();			
		}
		udpComm.clearBuf();
	}
}


void* reconnect(void* arg)
{
	hcSDK* hcsdk=static_cast<hcSDK*>(arg);

    while(1)
    {   
        for(int i=0;i<hcsdk->m_allDevNums;i++)  
        {   
            usleep(20000);  //20毫秒询问一次，1.2秒转一轮

            if(hcsdk->m_stat[i]==0) 
            {   
                sleep(1);   //1s后重新登录

                //重新登录
                NET_DVR_USER_LOGIN_INFO userLoginInfo{};    

                strcpy(userLoginInfo.sDeviceAddress,hcsdk->devInfos[i].devIP.c_str()); //设备IP
                userLoginInfo.wPort =8000;//stoi(hcsdk->devInfos[i].devPort); //设备端口
                strcpy(userLoginInfo.sUserName, hcsdk->devInfos[i].user.c_str()); //登录用户名
                strcpy(userLoginInfo.sPassword, hcsdk->devInfos[i].password.c_str()); //登录密码

                //设备信息, 输出参数
                NET_DVR_DEVICEINFO_V40 deviceInfo{};

                hcsdk->devInfos[i].loginRet=NET_DVR_Login_V40(&userLoginInfo, &deviceInfo);
				if (hcsdk->devInfos[i].loginRet < 0)
                {   
                    cout<<hcsdk->devInfos[i].devID<<" device login failed,error code:"<<NET_DVR_GetLastError()<<endl;
                }   
                else
                {   
                    cout<<hcsdk->devInfos[i].devID<<" device login successed!"<<endl;
                    hcsdk->m_stat[i]=1; //登录成功，状态置1
                }
            }
        }
    }	
	return NULL;
}

//解码回调函数
//pBuf为解码后的一帧视频流数据的指针
//nSize为解码后的一帧视频流数据的大小
//pFrameInfo为图像和声音结构体指针
//nReserved1和nReserved2为保留参数
void CALLBACK DecCBFun(int nPort, char* pBuf, int nSize, FRAME_INFO* pFrameInfo, void* nReserved1, int nReserved2)
{
    if(pFrameInfo->nType== T_YV12)	//视频流解码后的图像格式为YV12
    {
		//创建YV12格式的cv::Mat对象
		cv::Mat yv12Img(pFrameInfo->nHeight*3/2, pFrameInfo->nWidth, CV_8UC1, pBuf);

		//创建BGR格式的cv::Mat对象，用于存储格式转换后的图像
		cv::Mat bgrImg(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);

		//将YV12格式的图像转换为BGR格式的图像
		cv::cvtColor(yv12Img, bgrImg, cv::COLOR_YUV2BGR_YV12);

		//显示转换后的图像
/*
		if(portToIndex[nPort]==17)
		{

		cv::imshow("Video", bgrImg);
		cv::waitKey(10);

		}
*/

		//放入数组中
		//pthread_mutex_lock(&imgLocks[portToIndex[nPort]]);
		std::cout<<"aa11"<<std::endl;
		imgDevIDs[portToIndex[nPort]].img=bgrImg;
		//pthread_mutex_unlock(&imgLocks[portToIndex[nPort]]);
    }
}

//函数参数为系统自动传入
void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle,DWORD dwDataType,BYTE* pBuffer,DWORD dwBufSize,void* dwUser)
{
	HC* hc=(HC*)dwUser;

    if(dwDataType == NET_DVR_SYSHEAD) //视频流的系统头
    {
        if (!PlayM4_GetPort(&hc->nPort)) //获取播放库未使用的通道号
        {
            return;
        }
		portToIndex.insert(pair<LONG,int>(hc->nPort,hc->m_i));
		cout<<"nPort="<<hc->nPort<<endl;

        if(dwBufSize > 0)	//实时流数据大小，大于0
        {
            if (!PlayM4_SetStreamOpenMode(hc->nPort, STREAME_REALTIME))  //设置实时流播放模式
            {
                return;
            }

            if (!PlayM4_OpenStream(hc->nPort, pBuffer, dwBufSize, 1024*1024)) //打开流接口
            {
                return;
            }

            if (!PlayM4_SetDecCallBack(hc->nPort, DecCBFun))		//设置解码回调函数 只解码不显示
            {
                return;
            }

            if (!PlayM4_Play(hc->nPort, NULL))		//打开视频解码
            {
                return;
            }
        }
    }
    else if(dwDataType == NET_DVR_STREAMDATA)   //视频流的数据
    {
        if(dwBufSize>0 && hc->nPort!=-1)
        {
            PlayM4_InputData(hc->nPort, pBuffer, dwBufSize);
        }
    }
}

void* screenshot(void* arg)
{
	HC hc=*(HC*)arg;
	LONG lRealPlayHandle=NET_DVR_RealPlay_V40(hc.m_hcsdk->devInfos[hc.m_i].loginRet,
										 	  &hc.m_hcsdk->previewInfos[hc.m_i],
										 	  g_RealDataCallBack_V30,
										 	  arg);		
	if(lRealPlayHandle<0)
		std::cout<<"RealPlay_V40 err:"<<NET_DVR_GetLastError()<<std::endl;
/*
			//把ImgInfo结构体数组放进模型中
			pthread_mutex_lock(&imgLocks[hc.m_i]);
			imgDevIDs[hc.m_i].img=imgDevID.img;
			pthread_mutex_unlock(&imgLocks[hc.m_i]);
			imgDevIDs[hc.m_i].devID=midDevIDs[hc.m_i].devID;

			pthread_mutex_lock(&mutex2);    
			thread_status[hc.m_i]=1;
			while (thread_status[hc.m_i]!=0) 
			{   
				pthread_cond_wait(&cond2, &mutex2);
			}   
			pthread_mutex_unlock(&mutex2);

		else
		{

			pthread_mutex_lock(&mutex2);    
			thread_status[hc.m_i]=1;
			while (thread_status[hc.m_i]!=0) 
			{   
				pthread_cond_wait(&cond2, &mutex2);
			}   
			pthread_mutex_unlock(&mutex2);

			
		}

*/
	return NULL;
}

int main()
{
	//初始化DEVNUMS把图像锁
	for(int i=0; i<DEVNUMS; i++)
	{
		int ret = pthread_mutex_init(&imgLocks[i], NULL);
		if(ret!=0)	//返回非0，失败
			sys_err("pthread_mutex_init() imgLocks[i] err");
	}

	hcSDK hcsdk("devInfo.json");	//初始化devInfos数组容器，初始化midDevIDs数组容器

	hcsdk.loginDev();	//全部设备登录一遍，如果一台设备都没登录上，会一直重复登录

	const int allDevNums=hcsdk.m_allDevNums;

	imgDevIDs.resize(allDevNums);	//里面的所有元素都会被初始化，Mat为空矩阵，string为空字符串

	//创建线程，登录失败的设备，定期重连
	pthread_t tid1;
	int ret=pthread_create(&tid1,NULL,reconnect,&hcsdk);
	if(ret==-1)
		sys_err("pthread_create() reconnect err");

	//创建DEVNUMS个截图子线程	
	HC hcs[DEVNUMS];
	for(int i=0;i<DEVNUMS;i++)
	{
		hcs[i].m_hcsdk=&hcsdk;		
		hcs[i].m_i=i;		
	}
	pthread_t tid[DEVNUMS];
	for(int i=0; i<DEVNUMS; i++)
	{
		int ret=pthread_create(&tid[i],NULL,screenshot,&hcs[i]);
		if(ret==-1)
			sys_err("pthread_create() screenshot err");
	}


	//创建服务器线程
	pthread_t tid2;
	ret=pthread_create(&tid2,NULL,handler,&hcsdk);
	if(ret==-1)
		sys_err("pthread_create() handler err");

	YoloNas model("../yolonas.engine",allDevNums,"imgParam.json");	//创建模型对象，初始化并加载引擎

	sleep(3);	

	//首次推理需要确保每个设备都截取到图片，不然会出错
	while(1)
	{
		usleep(50000);

		cout<<"推理前"<<endl;
		model.detection();
		cout<<"推理后"<<endl;

	}

	return 0;
}
