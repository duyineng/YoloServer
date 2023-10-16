#pragma once
#include<string>
#include"info.h"
#include<unistd.h>
#include<opencv2/imgcodecs.hpp>
   #include<opencv2/imgproc.hpp>
   #include<opencv2/highgui.hpp>
  #include<string.h>
#include<arpa/inet.h>
using namespace std;
class UdpComm
{
public:
	UdpComm();
	~UdpComm();
	
	int recvReq();
	void getAlmStat(DetectInfo* detectInfos);
	void saveAlmInfo1(DetectInfo* detecAlarm,int size,DetectInfo* detectInfos);
	void saveAlmInfo2(DetectInfo* detecAlarm,int size,DetectInfo* detectInfos);

	//对应devID[0]有告警，devID[1]无告警的情况
	void writeXML1(DetectInfo* detecAlarm);

	//对应devID[0]无告警，devID[1]有告警的情况
	void writeXML2(DetectInfo* detecAlarm);

	//对应devID[0]和devID[1]都有告警的情况
	void writeXML3(DetectInfo* detecAlarm1,DetectInfo* detecAlarm2);
	
	//对应devID[0]和devID[1]都没有告警的情况
	void writeXML4();
	
	void clearBuf();
	void getTwoDevID(int allDevrNnums);
public:
	struct sockaddr_in clit_addr;	
    socklen_t clit_len=sizeof(clit_addr);	

	int m_fd;
	char buf[64]={0};
	char str[INET_ADDRSTRLEN]={0};  //保存客户端IP
    char char_arrMid[8]={0};
    char char_arrSeq[32]={0};

	string devID[2];	//会自动被初始化为空字符串，不需要手动初始化

	int alarm[2]={0};
    int num[2]={0};

	//DetectInfo* detecAlarm1;
	//DetectInfo* detecAlarm2;
};
