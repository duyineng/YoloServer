#pragma once

#include<opencv2/imgcodecs.hpp>
#include<opencv2/imgproc.hpp>
#include<opencv2/highgui.hpp>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<vector>
#include<pthread.h>
#include<unistd.h>
#include<string.h>
#include<string>
#include<iostream>
#include<fstream>
#include<json/json.h>
#include<stdio.h>
#include<fcntl.h>
#include<ctype.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<mxml.h>

#include"base64.h"
#include"HCNetSDK.h"
#include"yolo.hh"
#include"info.h"
#include"hcSDK.h"
#include"UdpComm.h"
#include"PlayM4.h"

int sys_err(const char* str)
{
    perror(str);

    exit(1);
}
