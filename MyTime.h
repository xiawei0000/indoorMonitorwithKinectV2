#pragma once
#define MYTIMER_H  
#define  NOMINMAX
#include <windows.h>  
#include<string>
#include <iostream>
#include <fstream> 
using namespace  std;
class MyTime
{

private:
	LONGLONG _freq;
	LARGE_INTEGER _begin;
	LARGE_INTEGER _end;
	bool usingFile;
	bool usingCmd;
	char * filename;
	//	FILE* out;
	//ofstream  out; 输入流作为 类的成员 如何初始化
public:
	long costTime;
	MyTime::MyTime()
	{
		LARGE_INTEGER tmp;
		QueryPerformanceFrequency(&tmp);
		_freq = tmp.QuadPart;
		costTime = 0;
		usingFile = false;
		usingCmd = false;
	}
	MyTime::MyTime(char *  filename)
	{
		this->filename = filename;
		usingFile = true;
		usingCmd = true;
		//	out = fstream(filename, ios_base::ate);
		//out = fopen(filename, "at");
		LARGE_INTEGER tmp;
		QueryPerformanceFrequency(&tmp);
		_freq = tmp.QuadPart;
		costTime = 0;
	}
	void MyTime::show(char * showname)
	{
		fstream out(filename, ios_base::out | ios_base::app);
		if (out &&usingFile)
		{
			out << showname << "耗时: ";
			out << costTime << "ms" << endl;
			//fprintf(out, "%s  耗时:  %d ms\n",showname, costTime);
			//fclose(out);
		}
		out.close();
		if (usingCmd)
		{
			cout << showname << "耗时: ";
			cout << costTime << endl;
			//fprintf(out, " %d\n", costTime);
		}
	}

	void MyTime::Start()
	{
		QueryPerformanceCounter(&_begin);
	}
	void MyTime::End()
	{
		QueryPerformanceCounter(&_end);
		costTime = (long)((_end.QuadPart - _begin.QuadPart) * 1000 / _freq);// ms

	}
	void MyTime::Reset()            // 计时清0  
	{
		costTime = 0;
	}
};

