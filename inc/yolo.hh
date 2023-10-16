#pragma once
#include<NvInfer.h>
#include<iostream>
#include<memory>
#include<opencv2/core.hpp>
#include"info.h"

class Logger : public nvinfer1::ILogger 
{
public:
    explicit Logger(Severity severity = Severity::kWARNING) : reportable_severity(severity) {}

    void log(Severity severity, const char* msg) noexcept 
	{
        if (severity > reportable_severity) 
		{
            return;
        }
        switch (severity) 
		{
            case Severity::kINTERNAL_ERROR:
                std::cerr << "INTERNAL_ERROR: ";
                break;
            case Severity::kERROR:
                std::cerr << "ERROR: ";
                break;
            case Severity::kWARNING:
                std::cerr << "WARNING: ";
                break;
            case Severity::kINFO:
                std::cerr << "INFO: ";
                break;
            default:
                std::cerr << "UNKNOWN: ";
                break;
        }
        std::cerr << msg << std::endl;
    }
    Severity reportable_severity;
};

struct alignas(float) Detection 
{
   float box[4];
   float score;
   float classes;
};

class YoloNas 
{
public:
    YoloNas(const std::string& plan_path,unsigned int max_batch_size,const char* filename);
    ~YoloNas();

    void detection();
private:
    void LoadEngine(const std::string& engine_file);
    void nms(std::vector<Detection>& res, int batch, float conf_thresh, float nms_thresh);

    Logger g_logger_;
    cudaStream_t stream_;
    nvinfer1::ICudaEngine* engine_;
    nvinfer1::IExecutionContext* context_;
	nvinfer1::IRuntime* runtime;

    float radtio_w;
    float radtio_h;

    uint8_t* img_host = nullptr;
    uint8_t* img_device = nullptr;
    
    void* buffers_[4];	//buffers_为指针数组首地址
    int buffer_size_[4];


    std::vector<float> boxes_;
    std::vector<float> scores_;
    std::vector<float> classes_;

    std::vector<std::vector<Detection>> detections; 
	
	unsigned int MAX_BATCH_SIZE;

	unsigned int INPUT_H;
	unsigned int INPUT_W;
	unsigned int RAW_IMG_H;
	unsigned int RAW_IMG_W;
	unsigned int MAX_IMAGE_INPUT_SIZE_THRESH;
	unsigned int MAX_COL_BLOCKS;      

	float CONF_THR;      
	float IOU_THR;      
};
