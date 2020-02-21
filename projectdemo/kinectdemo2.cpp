//////////直出深度信息进行Canny检测和标记
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <stack>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp> 
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>  
#include <Kinect.h>


using namespace cv;
using namespace std;

//自建函数声明
void on_ContoursChange(int, void*);


#define WINDOW_NAME1 "OriginDepthImage"
#define WINDOW_NAME2 "FliterImage"
#define WINDOW_NAME3 "CannyTest"
#define WINDOW_NAME4 "ContoursShow"  
#define WINDOW_NAME5 "BinarizatedImage"
#define WINDOW_NAME6 "EqualizeImage"

//多边形包围准备
Mat g_srcImage;
Mat g_grayImage;
int g_nThresh = 50;//阈值
int g_nMaxThresh = 255;//阈值最大值
RNG g_rng(12345);//随机数生成器
int g_FilterValue = 10;//滤波参数
cv::Mat mFliterImg;//滤波效果图
cv::Mat mCanny;
cv::Mat dilate_result;
//腐蚀与膨胀
Mat element = getStructuringElement(MORPH_RECT, Size(8, 8));


//滤波函数声明
static void on_BoxFilter(int, void *);		//方框滤波
static void on_MeanBlur(int, void *);		//均值块滤波器
static void on_GaussianBlur(int, void *);		//高斯滤波器
static void on_MedianBlur(int, void *);		//中值滤波器
static void on_BilateralFilter(int, void *);		//双边滤波器


//识别函数主体
int main(void)
{

	// 获取感应器
	IKinectSensor* pSensor = nullptr;
	GetDefaultKinectSensor(&pSensor);

	// 打开感应器
	pSensor->Open();

	// 取得深度数据
	IDepthFrameSource* pFrameSource = nullptr;
	pSensor->get_DepthFrameSource(&pFrameSource);

	// 取得深度数据的描述信息（宽、高）
	int        iWidth = 0;
	int        iHeight = 0;
	
	IFrameDescription* pFrameDescription = nullptr;
	pFrameSource->get_FrameDescription(&pFrameDescription);
	pFrameDescription->get_Width(&iWidth);
	pFrameDescription->get_Height(&iHeight);
	pFrameDescription->Release();
	pFrameDescription = nullptr;

	// 取得感应器的“可靠深度”的最大、最小距离
	UINT16 uDepthMin = 0, uDepthMax = 0;
	pFrameSource->get_DepthMinReliableDistance(&uDepthMin);
	pFrameSource->get_DepthMaxReliableDistance(&uDepthMax);
	cout << "Reliable Distance: "
		<< uDepthMin << " – " << uDepthMax << endl;

	// 建立图像矩阵，mDepthImg用来存储16位的图像数据，直接用来显示会接近全黑
	//不方便观察，用mImg8bit转换成8位再显示
	cv::Mat mDepthImg(iHeight, iWidth, CV_16UC1);
	cv::Mat mImg8bit(iHeight, iWidth, CV_8UC1);
	cv::Mat mBlackorWhite;// 二值化图像
	//cv::Mat dilate_result;
	//cv::Mat mCanny;
	cv::Mat mEqualize;
	cv::Mat mMinCountors;
	
	//cv::namedWindow("DepthImage");
	cv::namedWindow("CannyTest");
	


	// 打开深度数据阅读器
	IDepthFrameReader* pFrameReader = nullptr;
	pFrameSource->OpenReader(&pFrameReader);

	//定义一个矢量结构lines用于存放得到的线段矢量集合
	vector<Vec4i> lines;

	// 主循环
	while (1)
	{
		// 取得最新数据
		IDepthFrame* pFrame = nullptr;
		if (pFrameReader->AcquireLatestFrame(&pFrame) == S_OK)
		{
			// 把数据存入16位图像矩阵中
			pFrame->CopyFrameDataToArray(iWidth * iHeight,
				reinterpret_cast<UINT16*>(mDepthImg.data));// 强制转换数据类型，把16位转换成8位;要改变显示的颜色和效果，就改变从mDepthImg到mImg8bit的转换
			
			mDepthImg.convertTo(mImg8bit, CV_8U, 255.0f / uDepthMax);// converto()第一个参数是输出矩阵，第二个是转换类型，第三个是缩放因子，其中4500是深度数据的最大距离
			


			cvtColor(mImg8bit, g_grayImage, COLOR_GRAY2BGR);// 转为灰度图像
			//equalizeHist(g_grayImage, mEqualize);// 直方图均衡化

			//方框滤波
			//boxFilter(g_grayImage, mFliterImg, -1, Size(g_FilterValue + 1, g_FilterValue + 1));
			//均值滤波
			//blur(g_grayImage, mFliterImg, Size(g_FilterValue + 1, g_FilterValue + 1), Point(-1, -1));
			//高斯滤波
			//GaussianBlur(mBlackorWhite, mFliterImg, Size(g_FilterValue * 2 + 1, g_FilterValue * 2 + 1), 0, 0);
			//中值滤波
			medianBlur(g_grayImage, mFliterImg, g_FilterValue * 2 + 1);
			//双边滤波
			//bilateralFilter(mBlackorWhite, mFliterImg, g_FilterValue, g_FilterValue * 2, g_FilterValue / 2);

			threshold(mFliterImg, mBlackorWhite, 20, 255, CV_THRESH_BINARY);// 转化为自定范围的二值

			dilate(mBlackorWhite, dilate_result, element);// 图像腐蚀



			Canny(dilate_result, mCanny, 0, 30, 3);// 处理后图像进行边缘检测
			//g_grayImage = mImg8bit;
			

			cv::namedWindow(WINDOW_NAME1, WINDOW_AUTOSIZE);
			cv::namedWindow(WINDOW_NAME2, WINDOW_AUTOSIZE);
			cv::namedWindow(WINDOW_NAME3, WINDOW_AUTOSIZE);
			cv::namedWindow(WINDOW_NAME5, WINDOW_AUTOSIZE);
			//cv::namedWindow(WINDOW_NAME6, WINDOW_AUTOSIZE);
			cv::imshow(WINDOW_NAME1, mImg8bit);
			cv::imshow(WINDOW_NAME2, mFliterImg);
			cv::imshow(WINDOW_NAME3, mCanny);
			cv::imshow(WINDOW_NAME5, mBlackorWhite);
			//cv::imshow(WINDOW_NAME6, mEqualize);

			// 改变console字体颜色
			system("color 1F");

			// 设置滚动条并调用一次回调函数
			//createTrackbar(" 阈值:", WINDOW_NAME1, &g_nThresh, g_nMaxThresh, on_ContoursChange);
			on_ContoursChange(0, 0);
  
			//

			
			pFrame->Release();
		}

		if (cv::waitKey(30) == VK_ESCAPE) {
			break;
		}
	}

	// 释放变量pFrameReader
	pFrameReader->Release();
	pFrameReader = nullptr;

	// 释放变量pFramesSource
	pFrameSource->Release();
	pFrameSource = nullptr;

	// 关闭感应器
	pSensor->Close();

	// 释放感应器
	pSensor->Release();
	pSensor = nullptr;


	return 0;
}



////////////////////////////// 声明函数描述/////////////////////////////
//多边形逼近函数
void on_ContoursChange(int, void*)
{
	//定义一些参数
	Mat threshold_output; 
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	// 使用Threshold检测边缘
	threshold(mCanny, threshold_output, g_nThresh, 255, THRESH_BINARY);

	// 找出轮廓
	findContours(threshold_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

	// 多边形逼近轮廓 + 获取矩形和圆形边界框
	vector<vector<Point> > contours_poly(contours.size());
	vector<Rect> boundRect(contours.size());
	vector<Point2f>center(contours.size());
	vector<float>radius(contours.size());

	//一个循环，遍历所有部分，进行本程序最核心的操作
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);//用指定精度逼近多边形曲线 
		boundRect[i] = boundingRect(Mat(contours_poly[i]));//计算点集的最外面（up-right）矩形边界
		minEnclosingCircle(contours_poly[i], center[i], radius[i]);//对给定的 2D点集，寻找最小面积的包围圆形 
	}

	// 绘制多边形轮廓 + 包围的矩形框 + 圆形框
	Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);
	for (int unsigned i = 0; i<contours.size(); i++)
	{
		Scalar color = Scalar(g_rng.uniform(0, 255), g_rng.uniform(0, 255), g_rng.uniform(0, 255));//随机设置颜色
		drawContours(drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point());//绘制轮廓
		rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);//绘制矩形
	}

	// 显示效果图窗口
	cv::namedWindow(WINDOW_NAME4, WINDOW_AUTOSIZE);
	cv::imshow(WINDOW_NAME4, drawing);
}