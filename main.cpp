#include"main.h"
#include <opencv2/highgui.hpp>
#include"PlayM4.h"

#define DEVNUMS 14
using namespace std;

struct HC
{
    hcSDK* m_hcsdk;
    int m_i;
	LONG nPort;
};

extern unsigned int detectRecNums;	//在yolo.cc中定义	
extern vector<MidDevID> midDevIDs;	//在hcSDK.cpp中定义

pthread_mutex_t mutex1=PTHREAD_MUTEX_INITIALIZER;	//在数据段上定义并初始化互斥锁，程序结束自动释放，无需手动释放
pthread_mutex_t ImgLocks[DEVNUMS];	//在数据段上定义互斥锁数组，没有初始化

pthread_mutex_t mutex2=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2=PTHREAD_COND_INITIALIZER;

vector<ImgDevID> imgDevIDs;

DetectInfo detectInfos[60*20];

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
/*
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
                NET_DVR_USER_LOGIN_INFO loginInfo{};    //创建登录信息结构体

                loginInfo.bUseAsynLogin=0; //同步登录方式
                strcpy(loginInfo.sDeviceAddress,hcsdk->devInfos[i].devIP.c_str()); //设备IP地址
                loginInfo.wPort =8000;//stoi(hcsdk->devInfos[i].devPort); //设备端口
                strcpy(loginInfo.sUserName, hcsdk->devInfos[i].user.c_str()); //设备登录用户名
                strcpy(loginInfo.sPassword, hcsdk->devInfos[i].password.c_str()); //设备登录密码

                //设备信息, 输出参数
                NET_DVR_DEVICEINFO_V40 deviceInfo{};

                hcsdk->devInfos[i].loginRet=NET_DVR_Login_V40(&loginInfo, &deviceInfo);
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
*/
	return NULL;
}

void CALLBACK DecCBFun(int nPort, char * pBuf, int nSize, FRAME_INFO * pFrameInfo, void* nReserved1, int nReserved2)
{
    long lFrameType = pFrameInfo->nType;

    if(lFrameType == T_YV12)
    {
/*
        cv::Mat pImg(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3, pBuf);
        cv::imshow("Video", pImg);
        cv::waitKey(10);
*/
	// 创建一个YV12格式的cv::Mat对象
    cv::Mat yv12Img(pFrameInfo->nHeight * 3 / 2, pFrameInfo->nWidth, CV_8UC1, pBuf);

    // 创建一个BGR格式的cv::Mat对象，用于存储转换后的图像
    cv::Mat bgrImg(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);

    // 将YV12格式的图像转换为BGR格式
    cv::cvtColor(yv12Img, bgrImg, cv::COLOR_YUV2BGR_YV12);

    // 显示转换后的图像
/*
	if(nPort==0)
{

std::cout << "Thread ID: " << nPort << std::endl;
    cv::imshow("Video", bgrImg);
    cv::waitKey(10);
}
*/


    }
}

void CALLBACK fRealDataCallBack_V30(LONG lRealHandle,DWORD dwDataType,BYTE* pBuffer,DWORD dwBufSize,void* dwUser)

{
	HC hc=*(HC*)dwUser;

    if(dwDataType == NET_DVR_SYSHEAD) //如果是视频流数据
    {
        if (!PlayM4_GetPort(&( ((HC*)dwUser)->nPort))) //获取播放库未使用的通道号
        {
            return;
        }
		cout<<"nPort="<<((HC*)dwUser)->nPort<<endl;
		cout<<"errno="<<PlayM4_GetLastError<<endl;
        if(dwBufSize > 0)
        {
            if (!PlayM4_SetStreamOpenMode(((HC*)dwUser)->nPort, STREAME_REALTIME))  //设置实时流播放模式
            {
                return;
            }
            if (!PlayM4_OpenStream(((HC*)dwUser)->nPort, pBuffer, dwBufSize, 1024 * 1024)) //打开流接口
            {
                return;
            }
            //设置解码回调函数 只解码不显示
            if (!PlayM4_SetDecCallBack(((HC*)dwUser)->nPort, DecCBFun))
            {
			cout<<"进入dwDataType == NET_DVR_STREAMDATA"<<endl;
                return;
            }
            //打开视频解码
            if (!PlayM4_Play(((HC*)dwUser)->nPort, NULL))
            {
                return;
            }
            //打开音频解码,使用PlayM4函数
            if (!PlayM4_PlaySound(((HC*)dwUser)->nPort))
            {
                return;
            }
        }
    }
    else if(dwDataType == NET_DVR_STREAMDATA)   //视频流数据
    {
        if(dwBufSize > 0)
        {
            PlayM4_InputData(((HC*)dwUser)->nPort, pBuffer, dwBufSize);
        }
    }
	
}

void* screenshot(void* arg)
{
	
	HC hc=*(HC*)arg;
	LONG lRealPlayHandle;
	lRealPlayHandle=NET_DVR_RealPlay_V40(hc.m_hcsdk->devInfos[hc.m_i].loginRet,
										 &hc.m_hcsdk->struPlayInfos[hc.m_i],
										 fRealDataCallBack_V30,
										 arg);		
	if(lRealPlayHandle<0)
		std::cout<<"RealPlay_V40 err"<<std::endl;
	


		


/*
			//把ImgInfo结构体数组放进模型中
			pthread_mutex_lock(&ImgLocks[hc.m_i]);
			imgDevIDs[hc.m_i].img=imgDevID.img;
			pthread_mutex_unlock(&ImgLocks[hc.m_i]);
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

	//初始化40把截图锁
	for(int i=0;i<DEVNUMS;i++)
	{
		int ret=pthread_mutex_init(&ImgLocks[i],NULL);
		if(ret!=0)	//返回非0，失败
			sys_err("pthread_mutex_init ImgLocks err");
	}

	cout<<"初始化海康SDK前"<<endl;

	hcSDK hcsdk("devInfo.json");	//初始化devInfos数组容器，初始化midDevIDs数组容器

	cout<<"初始化海康SDK成功"<<endl;
	hcsdk.loginDev();	//全部设备登录一遍，如果一台设备都没登录上，会一直重复登录

	const int allDevNums=hcsdk.m_allDevNums;

	imgDevIDs.resize(allDevNums);	//里面的所有元素都会被初始化，Mat为空矩阵，string为空字符串

	cout<<"111"<<endl;
	//创建线程，登录失败的设备，定期重连
	pthread_t tid1;
	int ret=pthread_create(&tid1,NULL,reconnect,&hcsdk);
	if(ret==-1)
		sys_err("pthread_create() reconnect err");

	//创建40个截图子线程	
	HC hcs[DEVNUMS];
	for(int i=0;i<DEVNUMS;i++)
	{
		hcs[i].m_hcsdk=&hcsdk;		
		hcs[i].m_i=i;		
	}
	pthread_t tid[DEVNUMS];
	for(int i=0;i<DEVNUMS;i++)
	{
		int ret=pthread_create(&tid[i],NULL,screenshot,&hcs[i]);
	//	int ret=pthread_create(&tid[i],NULL,screenshot,&nPorts[i]);
		if(ret==-1)
			sys_err("pthread_create() screenshot err");
	}


	//创建服务器线程
	pthread_t tid2;
	ret=pthread_create(&tid2,NULL,handler,&hcsdk);
	if(ret==-1)
		sys_err("pthread_create() handler err");

	//YoloNas model("../yolonas.engine",allDevNums,"imgParam.json");	//创建模型对象，初始化并加载引擎

	//sleep(5);	

	//首次推理需要确保每个设备都截取到图片，不然会出错
	while(1)
	{
		usleep(50000);
/*
		cout<<"推理前"<<endl;
		model.detection();
		cout<<"推理后"<<endl;
*/
	}


	return 0;
}
