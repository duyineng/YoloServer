#include<opencv2/imgcodecs.hpp>
#include<opencv2/imgproc.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/opencv.hpp>
#include<string>
#include<json/json.h>
#include<fstream>
#include<unistd.h>
#include<mutex>
#include"HCNetSDK.h"
#include"hcSDK.h"
#include<stdlib.h>

using namespace Json;

vector<MidDevID> midDevIDs;

hcSDK::hcSDK(std::string filename)
{
	NET_DVR_Init();	//初始化海康SDK环境

	NET_DVR_SetConnectTime(5000, 655355);	//连接超时时间5s，连接次数655355
 	NET_DVR_SetReconnect(10000, true);	//间隔10s重现连接一次，true表示开启重连
	
	//打开json文件
    ifstream ifs("devInfo.json");   //创建ifstream流对象，流对象里面有文件的数据，默认该流对象对文件只有读权限
	if(!ifs.is_open())
	{
		std::cout<<R"(打开 “devInfo.json”文件 失败)"<<std::endl;

		exit(1);
	}
	
	//解析json文件
    Value root; 
    Reader read;
    bool bl=read.parse(ifs,root);
	if(!bl)
	{
		std::cout<<"read.parse() err，json文件格式写错"<<std::endl;

		exit(1);
	}	
    ifs.close();    //关闭文件，即使不调用这句，这个构造函数执行完，ifs也会被析构，然后关闭所打开的文件

    m_allDevNums=root.size();	//配置文件中填写的设备数量	
	if(m_allDevNums==0)
	{
		std::cout<<"m_allDevNums==0，json文件格式写错"<<std::endl;

		exit(1);
	}

	m_stat=new int[m_allDevNums];	//存放各个设备登录成功与否的标志位
    memset(m_stat,0,sizeof(int)*m_allDevNums);

	//配置文件的设备信息读到结构体数组中
    for(int i=0;i<m_allDevNums;i++) 
    {   
		//存入设备信息结构体数组中
        Value object = root[i];   //devInfo.json文件的最外层是json数组，root[i]就是指文件中的第i+1个数组对象
		DevInfo devInfo{};	//里面的string会被初始化为空，int会被初始化为0

        devInfo.mid=object["mid"].asString();   //机号  
        devInfo.devID=object["devID"].asString();     //设备号
        devInfo.devIP=object["devIP"].asString();   //设备的ip
        devInfo.devPort=object["devPort"].asString();   //设备端口
        devInfo.user=object["user"].asString(); //登录用户名
        devInfo.password=object["password"].asString(); //登录密码
        devInfo.channel=object["channel"].asString(); //登录通道号

		devInfos.push_back(devInfo);

		//存入机号设备号结构体数组中
        MidDevID tmp{}; 

        tmp.mid=devInfo.mid;    //机号
        tmp.devID=devInfo.devID;  //设备号

        midDevIDs.push_back(tmp);

		//存入预览信息结构体数组中
		NET_DVR_PREVIEWINFO previewInfo{};

		previewInfo.lChannel = stoi(devInfo.channel); //预览通道号
		previewInfo.dwStreamType = 0; //主码流
		previewInfo.dwLinkMode = 0; //TCP方式
		previewInfo.hPlayWnd = NULL; //播放窗口的句柄，为NULL表示预览画面不显示在任何窗口
		previewInfo.bBlocked = 1; // 阻塞取流方式
		
		previewInfos.push_back(previewInfo);		
    }
}

hcSDK::~hcSDK()
{
	delete[] m_stat;

	NET_DVR_Cleanup();
}

void hcSDK::loginDev()
{
	int flag=0;

    //所有设备登录一次
    do  
    {   
        for(int i=0; i<m_allDevNums; i++) 
        {   
            //用户登录信息
            NET_DVR_USER_LOGIN_INFO userLoginInfo{};    //登录信息结构体

            strcpy(userLoginInfo.sDeviceAddress, devInfos[i].devIP.c_str()); //设备IP
            userLoginInfo.wPort=stoi(devInfos[i].devPort); //设备端口
            strcpy(userLoginInfo.sUserName, devInfos[i].user.c_str()); //登录设备用户名
            strcpy(userLoginInfo.sPassword, devInfos[i].password.c_str()); //登录设备密码

            //设备信息 
            NET_DVR_DEVICEINFO_V40 deviceInfo{};

            devInfos[i].loginRet=NET_DVR_Login_V40(&userLoginInfo, &deviceInfo);
			if (devInfos[i].loginRet < 0)
            {   
                std::cout<<devInfos[i].devID<<"NET_DVR_Login_V40() err:"<<NET_DVR_GetLastError()<<std::endl;
            }
            else
            {
                std::cout<<devInfos[i].devID<<"NET_DVR_Login_V40() successed!"<<std::endl;
                m_stat[i]=1; //登录成功，状态置1
                flag=1;
            }
        }
    }
    while(flag!=1);	
}

