#pragma once 
#include<iostream>
#include"HCNetSDK.h"
#include<opencv2/imgcodecs.hpp>
#include<opencv2/imgproc.hpp>
#include<opencv2/highgui.hpp>
#include"base64.h"
#include<arpa/inet.h>
#include"hcSDK.h"

using namespace std;
/*
//设备信息
struct DevInfo
{
	string mid;	//机号
	string devID;	//设备号
	string devIP;	//所在NVR的IP
	string devPort;
	string user;	//登录NVR用户名
	string password;	//登录NVR密码
	LONG loginRet;
};
*/

struct ImgDevID
{
	cv::Mat img;
	string devID;

	ImgDevID()
	{
		cv::Mat img=cv::Mat::zeros(cv::Size(1080,720),CV_8UC3);
		std::cout <<"img:"<<img.rows<<"img:"<<img.cols << std::endl;
	}
};

struct DetectInfo
{
	char devID[8];
	int x;
	int y;
	int w;
	int h;

	//DetectInfo() :devID{0},x(0.0f),y(0.0f),w(0.0f),h(0.0f) {}
	DetectInfo() :devID{0},x(0),y(0),w(0),h(0) {}
};

struct MidDevID
{
	string mid;
	string devID;
};

struct box 
{
    float x;
    float y;
    float w;
    float h;
};
