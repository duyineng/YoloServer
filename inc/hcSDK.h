#pragma once

#include<opencv2/imgcodecs.hpp>
#include<opencv2/imgproc.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/opencv.hpp>
#include<string>
#include<vector>
#include<mutex>
#include"info.h"
#include"HCNetSDK.h"

using namespace std;

struct DevInfo
{
    string mid; //机号
    string devID;   //设备号
    string devIP;   //所在NVR的IP
    string devPort;
    string user;    //登录NVR用户名
    string password;    //登录NVR密码
    LONG loginRet;
	string channel;
};


class hcSDK
{
public:
	hcSDK(std::string filename);
	~hcSDK();

	void loginDev();	
	bool screenshot(std::string devID,LONG loginRet,int channel,char buf[],int size,unsigned int& sizeRet);
public:
	vector<DevInfo> devInfos;
	int m_allDevNums;
	int* m_stat;
	vector<NET_DVR_PREVIEWINFO>		struPlayInfos;
private:
    NET_DVR_JPEGPARA jpegPara={0xff,2}; //C+11新特性，类内就地初始化,效率最高
};
