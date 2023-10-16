#include"main.h"
#include <opencv2/highgui.hpp>

#define DEVNUMS 3
using namespace std;

struct HC
{
    hcSDK* m_hcsdk;
    int m_i;
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

void* screenshot(void* arg)
{
	while(1)
	{
		HC hc=*(HC*)arg;
		
		char buf[1024*1024]={0};
		unsigned int sizeRet=0;
		ImgDevID imgDevID{};    //初始化清零 

		auto start = std::chrono::system_clock::now();
		if(hc.m_hcsdk->screenshot(hc.m_hcsdk->devInfos[hc.m_i].devID,hc.m_hcsdk->devInfos[hc.m_i].loginRet,
								  stoi(hc.m_hcsdk->devInfos[hc.m_i].channel),buf,sizeof(buf),sizeRet))
		{
			auto end = std::chrono::system_clock::now();
    std::cout << "screenshot time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
			//截图成功
			cout<<"sizeRet="<<sizeRet<<endl;

			cv::Mat img(1,sizeRet,CV_8UC1,buf); //从buf中取数据，创建1行，sizeRet列，且单通道的原始JPEG数据
			imgDevID.img=cv::imdecode(img,cv::IMREAD_COLOR);  //对原始JPEG数据进行解码，解码后得到图像数据
			imgDevID.devID=hc.m_hcsdk->devInfos[hc.m_i].devID;



		if(stoi(midDevIDs[hc.m_i].devID)==2001)
            {   
                cv::imshow( "Display window",imgDevID.img );                // 在窗口中显示图像
                cv::waitKey(10); 
            }


			//把ImgInfo结构体数组放进模型中
			pthread_mutex_lock(&ImgLocks[hc.m_i]);
			imgDevIDs[hc.m_i].img=imgDevID.img;
			pthread_mutex_unlock(&ImgLocks[hc.m_i]);
			imgDevIDs[hc.m_i].devID=midDevIDs[hc.m_i].devID;
/*

			pthread_mutex_lock(&mutex2);    
			thread_status[hc.m_i]=1;
			while (thread_status[hc.m_i]!=0) 
			{   
				pthread_cond_wait(&cond2, &mutex2);
			}   
			pthread_mutex_unlock(&mutex2);
*/
		}
		else
		{
/*
			pthread_mutex_lock(&mutex2);    
			thread_status[hc.m_i]=1;
			while (thread_status[hc.m_i]!=0) 
			{   
				pthread_cond_wait(&cond2, &mutex2);
			}   
			pthread_mutex_unlock(&mutex2);
*/
			
		}

	}

	return NULL;
}

int main()
{
	cout<<"111"<<endl;

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
		if(ret==-1)
			sys_err("pthread_create() screenshot err");
	}

	//创建服务器线程
	pthread_t tid2;
	ret=pthread_create(&tid2,NULL,handler,&hcsdk);
	if(ret==-1)
		sys_err("pthread_create() handler err");

//	YoloNas model("../yolonas.engine",allDevNums,"imgParam.json");	//创建模型对象，初始化并加载引擎

	sleep(5);	

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
