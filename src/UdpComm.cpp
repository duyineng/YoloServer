#include<mxml.h>
#include"UdpComm.h"
#include"info.h"
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#define SERV_PORT 9530
extern unsigned int detectRecNums;

extern vector<MidDevID> midDevIDs;
UdpComm::UdpComm()
{
	//创建套接字文件描述符
	m_fd=socket(AF_INET,SOCK_DGRAM,0);    //SOCK_DGRAM代表报式协议，0代表报式协议中的UDP
	if(m_fd==-1)
	{
		perror("socket() err");
		exit(1);
	}

	//初始化服务器地址结构
	struct sockaddr_in serv_addr;
	bzero(&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(SERV_PORT);

	//将文件描述符和服务器地址结构进行绑定
	int ret=bind(m_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
	if(ret==-1)
	{
		perror("bind() err");
		exit(1);
	}
}

UdpComm::~UdpComm()
{
	close(m_fd);
}

int UdpComm::recvReq()
{
	int recvlen=recvfrom(m_fd,buf,sizeof(buf),0,(struct sockaddr*)&clit_addr,&clit_len);
	if(recvlen==-1)
	{   
		perror("recvfrom() err");
		return -1;
	}
	else if(recvlen==0)
	{   
		perror("client close");
		return 1;
	}
	else
	{
		printf("received from %s at PORT %d\n",
			inet_ntop(AF_INET,&clit_addr.sin_addr.s_addr,str,sizeof(str)), ntohs(clit_addr.sin_port));

		//提取mid和seq
		sscanf(buf,"ReqAlm?mid=%[^&]&seq=%s",char_arrMid,char_arrSeq);
	
		return 0;
	}
}

void UdpComm::getTwoDevID(int allDevNums)
{
	int flag=0;
	for(int i=0;i<allDevNums;i++)	
	{
		if( strcmp(midDevIDs[i].mid.c_str(),char_arrMid)==0 )
		{
			devID[flag++]=midDevIDs[i].devID;
			if(flag==2)
				break;
		}
	}
}

void UdpComm::getAlmStat(DetectInfo* detectInfos)
{
	for(int i=0;i<detectRecNums;i++)
	{
		for(int j=0;j<2;j++)
		{
			if( strcmp((const char*)detectInfos[i].devID,devID[j].c_str())==0 )
			{
				alarm[j]=1;
				num[j]++;
				break;
			}
		}

	}	
}

void UdpComm::saveAlmInfo1(DetectInfo* detecAlarm,int size,DetectInfo* detectInfos)
{
	for(int a=0,i=0;a<size,i<detectRecNums;i++)
	{
		//cout<<"a="<<a<<endl;
		if( strcmp((const char*)detectInfos[i].devID,devID[0].c_str())==0 )
		{
			strcpy(detecAlarm[a].devID,(const char*)detectInfos[i].devID);
			detecAlarm[a].x=detectInfos[i].x;
			detecAlarm[a].y=detectInfos[i].y;
			detecAlarm[a].w=detectInfos[i].w;
			detecAlarm[a].h=detectInfos[i].h;
			cout<<"detecAlarm1["<<a<<"].devID="<<detecAlarm[a].devID<<endl;
			cout<<"h="<<detectInfos[i].h<<endl;
			a++;
		}
	}	
}

void UdpComm::saveAlmInfo2(DetectInfo* detecAlarm,int size,DetectInfo* detectInfos)
{
	for(int a=0,i=0;a<size,i<detectRecNums;i++)
	{
		//cout<<"a="<<a<<endl;
		if( strcmp((const char*)detectInfos[i].devID,devID[1].c_str())==0 )
		{
			strcpy(detecAlarm[a].devID,(const char*)detectInfos[i].devID);
			detecAlarm[a].x=detectInfos[i].x;
			detecAlarm[a].y=detectInfos[i].y;
			detecAlarm[a].w=detectInfos[i].w;
			detecAlarm[a].h=detectInfos[i].h;
			cout<<"detecAlarm2["<<a<<"].devID="<<detecAlarm[a].devID<<endl;
			cout<<"h="<<detectInfos[i].h<<endl;
			a++;
		}
	}	
}

//对应devID[0]有告警，devID[1]无告警的情况
void UdpComm::writeXML1(DetectInfo* detecAlarm)
{
	//创建xml文件的根元素,并由root指针指向      
	mxml_node_t* root=mxmlNewXML("1.0");
	if(root==NULL)
	{
		cout<<"mxmlNewXML() err";
		return;	
	}

	//在根元素下创建Alarm子元素,并由Alarm指针指向 
	mxml_node_t* Alarm=mxmlNewElement(root,"Alarm");
	//为Alarm元素设置属性，并赋值
	mxmlElementSetAttr(Alarm,"mid",char_arrMid);
	mxmlElementSetAttr(Alarm,"seq",char_arrSeq);
	mxmlElementSetAttr(Alarm,"code"," ");

	//在Alarm元素下创建Item子元素
	mxml_node_t* Item=mxmlNewElement(Alarm,"Item");
	//为Item元素设置属性，并赋值
	mxmlElementSetAttr(Item,"id",devID[0].c_str());
	mxmlElementSetAttr(Item,"alarmtype",to_string(alarm[0]).c_str());

	//创建一个独立的vca元素
	mxml_node_t* vca=mxmlNewElement(NULL,"vca");
	//在vca元素下创建objects子元素
	mxml_node_t* objects=mxmlNewElement(vca,"objects");

	for(int j=0;j<num[0];j++)
	{   
		mxml_node_t* object=mxmlNewElement(objects,"object");
		mxml_node_t* id=mxmlNewElement(object,"id");
		mxmlNewText(id,0,detecAlarm[j].devID);

		mxml_node_t* bb=mxmlNewElement(object,"bb");
		mxml_node_t* x=mxmlNewElement(bb,"x");
		mxmlNewText(x,0,to_string(detecAlarm[j].x).c_str());

		mxml_node_t* y=mxmlNewElement(bb,"y");
		mxmlNewText(y,0,to_string(detecAlarm[j].y).c_str());

		mxml_node_t* w=mxmlNewElement(bb,"w");
		mxmlNewText(w,0,to_string(detecAlarm[j].w).c_str());

		mxml_node_t* h=mxmlNewElement(bb,"h");
		mxmlNewText(h,0,to_string(detecAlarm[j].h).c_str());
	}	

	char* tempXmlStr=mxmlSaveAllocString(vca,MXML_NO_CALLBACK);
	char encodeBase64[4096*2]={0};
	Base64Encode(reinterpret_cast<unsigned char*>(tempXmlStr),strlen(tempXmlStr),encodeBase64);
	mxml_node_t* cnt=mxmlNewElement(Item,"cnt");
	mxmlNewText(cnt,0,encodeBase64);

	free(tempXmlStr);
	mxmlDelete(vca);

	Item=mxmlNewElement(Alarm,"Item");
	mxmlElementSetAttr(Item,"id",devID[1].c_str());
	mxmlElementSetAttr(Item,"alarmtype",to_string(alarm[1]).c_str());

	char* XmlStr=mxmlSaveAllocString(root,MXML_NO_CALLBACK);
	sendto(m_fd,XmlStr,strlen(XmlStr),0,(struct sockaddr*)&clit_addr,clit_len);

	free(XmlStr);
	mxmlDelete(root);
}

//对应devID[0]无告警，devID[1]有告警的情况
void UdpComm::writeXML2(DetectInfo* detecAlarm)
{
	//创建xml文件的根元素,并由root指针指向      
	mxml_node_t* root=mxmlNewXML("1.0");

	//在根元素下创建Alarm子元素,并由Alarm指针指向 
	mxml_node_t* Alarm=mxmlNewElement(root,"Alarm");
	//为Alarm元素设置属性，并赋值
	mxmlElementSetAttr(Alarm,"mid",char_arrMid);
	mxmlElementSetAttr(Alarm,"seq",char_arrSeq);
	mxmlElementSetAttr(Alarm,"code"," ");

	mxml_node_t* Item=mxmlNewElement(Alarm,"Item");
	mxmlElementSetAttr(Item,"id",devID[0].c_str());
	mxmlElementSetAttr(Item,"alarmtype",to_string(alarm[0]).c_str());

	//在Alarm元素下创建Item子元素
	Item=mxmlNewElement(Alarm,"Item");
	//为Item元素设置属性，并赋值
	mxmlElementSetAttr(Item,"id",devID[1].c_str());
	mxmlElementSetAttr(Item,"alarmtype",to_string(alarm[1]).c_str());

	//创建一个独立的vca元素
	mxml_node_t* vca=mxmlNewElement(NULL,"vca");
	//在vca元素下创建objects子元素
	mxml_node_t* objects=mxmlNewElement(vca,"objects");

	for(int j=0;j<num[1];j++)
	{   
		mxml_node_t* object=mxmlNewElement(objects,"object");
		mxml_node_t* id=mxmlNewElement(object,"id");
		mxmlNewText(id,0,detecAlarm[j].devID);

		mxml_node_t* bb=mxmlNewElement(object,"bb");
		mxml_node_t* x=mxmlNewElement(bb,"x");
		mxmlNewText(x,0,to_string(detecAlarm[j].x).c_str());

		mxml_node_t* y=mxmlNewElement(bb,"y");
		mxmlNewText(y,0,to_string(detecAlarm[j].y).c_str());

		mxml_node_t* w=mxmlNewElement(bb,"w");
		mxmlNewText(w,0,to_string(detecAlarm[j].w).c_str());

		mxml_node_t* h=mxmlNewElement(bb,"h");
		mxmlNewText(h,0,to_string(detecAlarm[j].h).c_str());
	}	

	char* tempXmlStr=mxmlSaveAllocString(vca,MXML_NO_CALLBACK);
	char encodeBase64[4096*2]={0};
	Base64Encode(reinterpret_cast<unsigned char*>(tempXmlStr),strlen(tempXmlStr),encodeBase64);
	mxml_node_t* cnt=mxmlNewElement(Item,"cnt");
	mxmlNewText(cnt,0,encodeBase64);

	free(tempXmlStr);
	mxmlDelete(vca);

	char* XmlStr=mxmlSaveAllocString(root,MXML_NO_CALLBACK);
	sendto(m_fd,XmlStr,strlen(XmlStr),0,(struct sockaddr*)&clit_addr,clit_len);

	free(XmlStr);
	mxmlDelete(root);
}

//对应devID[0]和devID[1]都有告警的情况
void UdpComm::writeXML3(DetectInfo* detecAlarm1,DetectInfo* detecAlarm2)
{
	mxml_node_t* root=mxmlNewXML("1.0");
	if(root==NULL)
	{
		cout<<"mxmlNewXML() err";
		return;	
	}
		

	mxml_node_t* Alarm=mxmlNewElement(root,"Alarm");
	mxmlElementSetAttr(Alarm,"mid",char_arrMid);
	mxmlElementSetAttr(Alarm,"seq",char_arrSeq);
	mxmlElementSetAttr(Alarm,"code"," ");

	mxml_node_t* Item=mxmlNewElement(Alarm,"Item");
	mxmlElementSetAttr(Item,"id",devID[0].c_str());
	mxmlElementSetAttr(Item,"alarmtype",to_string(alarm[0]).c_str());

	//有告警，就写信息
	mxml_node_t* vca=mxmlNewElement(NULL,"vca");
	mxml_node_t* objects=mxmlNewElement(vca,"objects");	

	for(int j=0;j<num[0];j++)
	{
		mxml_node_t* object=mxmlNewElement(objects,"object");
		mxml_node_t* id=mxmlNewElement(object,"id");
		mxmlNewText(id,0,detecAlarm1[j].devID);

		mxml_node_t* bb=mxmlNewElement(object,"bb");
		mxml_node_t* x=mxmlNewElement(bb,"x");
		mxmlNewText(x,0,to_string(detecAlarm1[j].x).c_str());

		mxml_node_t* y=mxmlNewElement(bb,"y");
		mxmlNewText(y,0,to_string(detecAlarm1[j].y).c_str());

		mxml_node_t* w=mxmlNewElement(bb,"w");
		mxmlNewText(w,0,to_string(detecAlarm1[j].w).c_str());

		mxml_node_t* h=mxmlNewElement(bb,"h");
		mxmlNewText(h,0,to_string(detecAlarm1[j].h).c_str());
	}

	char* tempXmlStr=mxmlSaveAllocString(vca,MXML_NO_CALLBACK);
	char encodeBase64[4096*2]={0};
	Base64Encode(reinterpret_cast<unsigned char*>(tempXmlStr),strlen(tempXmlStr),encodeBase64);
	mxml_node_t* cnt=mxmlNewElement(Item,"cnt");
	mxmlNewText(cnt,0,encodeBase64);

	free(tempXmlStr);
	mxmlDelete(vca);

	Item=mxmlNewElement(Alarm,"Item");
	mxmlElementSetAttr(Item,"id",devID[1].c_str());
	mxmlElementSetAttr(Item,"alarmtype",to_string(alarm[1]).c_str());

	//有告警，就写信息
	vca=mxmlNewElement(NULL,"vca");
	objects=mxmlNewElement(vca,"objects");	


	for(int j=0;j<num[1];j++)
	{
		mxml_node_t* object=mxmlNewElement(objects,"object");
		mxml_node_t* id=mxmlNewElement(object,"id");
		mxmlNewText(id,0,detecAlarm2[j].devID);

		mxml_node_t* bb=mxmlNewElement(object,"bb");
		mxml_node_t* x=mxmlNewElement(bb,"x");
		mxmlNewText(x,0,to_string(detecAlarm2[j].x).c_str());

		mxml_node_t* y=mxmlNewElement(bb,"y");
		mxmlNewText(y,0,to_string(detecAlarm2[j].y).c_str());

		mxml_node_t* w=mxmlNewElement(bb,"w");
		mxmlNewText(w,0,to_string(detecAlarm2[j].w).c_str());

		mxml_node_t* h=mxmlNewElement(bb,"h");
		mxmlNewText(h,0,to_string(detecAlarm2[j].h).c_str());
	}

	tempXmlStr=mxmlSaveAllocString(vca,MXML_NO_CALLBACK);
	bzero(encodeBase64,sizeof(encodeBase64));
	Base64Encode(reinterpret_cast<unsigned char*>(tempXmlStr),strlen(tempXmlStr),encodeBase64);
	cnt=mxmlNewElement(Item,"cnt");
	mxmlNewText(cnt,0,encodeBase64);

	free(tempXmlStr);
	mxmlDelete(vca);

	char* XmlStr=mxmlSaveAllocString(root,MXML_NO_CALLBACK);
	sendto(m_fd,XmlStr,strlen(XmlStr),0,(struct sockaddr*)&clit_addr,clit_len);

	free(XmlStr);
	cout<<"释放前"<<endl;
	mxmlDelete(root);
	cout<<"释放后"<<endl;
}

//对应devID[0]和devID[1]都没有告警的情况
void UdpComm::writeXML4()
{
	mxml_node_t* root=mxmlNewXML("1.0");

	mxml_node_t* Alarm=mxmlNewElement(root,"Alarm");
	mxmlElementSetAttr(Alarm,"mid",char_arrMid);
	mxmlElementSetAttr(Alarm,"seq",char_arrSeq);
	mxmlElementSetAttr(Alarm,"code"," ");

	mxml_node_t* Item=mxmlNewElement(Alarm,"Item");
	mxmlElementSetAttr(Item,"id",devID[0].c_str());
	mxmlElementSetAttr(Item,"alarmtype",to_string(alarm[0]).c_str());

	Item=mxmlNewElement(Alarm,"Item");
	mxmlElementSetAttr(Item,"id",devID[1].c_str());
	mxmlElementSetAttr(Item,"alarmtype",to_string(alarm[1]).c_str());

	char* XmlStr=mxmlSaveAllocString(root,MXML_NO_CALLBACK);
	sendto(m_fd,XmlStr,strlen(XmlStr),0,(struct sockaddr*)&clit_addr,clit_len);

	free(XmlStr);
	mxmlDelete(root);
}

void UdpComm::clearBuf()
{
	
	bzero(buf,sizeof(buf));
	bzero(str,sizeof(str));
	bzero(char_arrMid,sizeof(char_arrMid));
	bzero(char_arrSeq,sizeof(char_arrSeq));
	
	num[0]=0;
	num[1]=0;
	alarm[0]=0;
	alarm[1]=0;
}

