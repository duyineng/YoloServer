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
using namespace std;

vector<MidDevID> midDevIDs;
hcSDK::hcSDK(std::string filename)
{
	NET_DVR_Init();	//初始化海康SDK环境
	
	//打开json文件
    ifstream ifs("devInfo.json");   //创建ifs流对象，对象里面有文件的数据，默认该对象对文件只有读权限
	if(!ifs.is_open())
	{
		cout<<"open devInfo.json file err"<<endl;
		exit(1);
	}
	
    Reader read;
    Value root; 
    bool bl=read.parse(ifs,root);
	if(!bl)
	{
		std::cout<<"read.parse() err"<<std::endl;
		exit(1);
	}	

    ifs.close();    //关闭文件，即使不调用这句，这个函数执行完，ifs也会自动被析构，然后关闭文件

    m_allDevNums=root.size();	//配置文件中填写的设备数量	
	if(m_allDevNums==0)
	{
		std::cout<<"err:m_allDevNums==0"<<std::endl;
		exit(1);
	}

	m_stat=new int[m_allDevNums];	//登录成功与失败标志位数组
    memset(m_stat,0,sizeof(int)*m_allDevNums);

	//配置文件的设备信息读到结构体数组中
    for(int i=0;i<m_allDevNums;i++) 
    {   
        Value object=root[i];   //文件形式是一个大的json数组
		DevInfo devInfo{};	//里面的string会被初始化为空，int会被初始化为0

        devInfo.mid=object["mid"].asString();   //机号  
        devInfo.devID=object["devID"].asString();     //设备号
        devInfo.devIP=object["devIP"].asString();   //设备的ip
        devInfo.devPort=object["devPort"].asString();   //设备端口
        devInfo.user=object["user"].asString(); //登录用户名
        devInfo.password=object["password"].asString(); //登录密码
        devInfo.channel=object["channel"].asString(); //登录密码

		devInfos.push_back(devInfo);

        MidDevID tmp{}; 
        tmp.mid=devInfo.mid;    //机号
        tmp.devID=devInfo.devID;  //设备号

        midDevIDs.push_back(tmp);
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
        for(unsigned int i=0;i<m_allDevNums;i++) 
        {   
            //登录信息
            NET_DVR_USER_LOGIN_INFO loginInfo{};    //创建登录信息结构体，并初始化清零

            loginInfo.bUseAsynLogin=0; //同步登录方式(程序会等待设备响应并返回登录结果后再继续执行后续操作)
            strcpy(loginInfo.sDeviceAddress,devInfos[i].devIP.c_str()); //设备IP地址
            loginInfo.wPort=stoi(devInfos[i].devPort); //设备端口
            strcpy(loginInfo.sUserName,devInfos[i].user.c_str()); //设备登录用户名
            strcpy(loginInfo.sPassword,devInfos[i].password.c_str()); //设备登录密码

            //设备信息, 输出参数
            NET_DVR_DEVICEINFO_V40 deviceInfo{};

            devInfos[i].loginRet=NET_DVR_Login_V40(&loginInfo, &deviceInfo);
			if (devInfos[i].loginRet < 0)
            {   
                cout<<devInfos[i].devID<<" device login failed,error code:"<<NET_DVR_GetLastError()<<endl;
            }
            else
            {
                cout<<devInfos[i].devID<<" device login successed!"<<endl;
                m_stat[i]=1; //登录成功，状态置1
                flag=1;
            }
        }
    }
    while(flag!=1);	
}

bool hcSDK::screenshot(string devID,LONG loginRet,int channel,char buf[],int size,unsigned int& sizeRet)
{
    if (!NET_DVR_CaptureJPEGPicture_NEW(loginRet,channel,&(this->jpegPara),buf,size,&sizeRet))
    {
        cout<<"CaptureJPEGPicture"<<devID<<" device failed,error code:"<<NET_DVR_GetLastError()<<endl;
        return false;
    }
    else
    {
        cout<<"CaptureJPEGPicture"<<devID<<" success!"<<endl;
        return true;
    }
}
