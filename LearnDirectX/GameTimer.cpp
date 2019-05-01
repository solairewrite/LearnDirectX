#include <windows.h> // QueryPerformanceFrequency
#include "GameTimer.h"

GameTimer::GameTimer() : mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec); // include <windows.h>
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

// 返回调用Reset()后的总时间,不包括停止的时间
float GameTimer::TotalTime() const
{
	//				   |<--paused time-->|
	// -----*----------*-----------------*----------*-------------*-----> time
	//  mBaseTime	mStopTime		startTime	mStopTime		mCurrTime
	if (mStopped)
	{
		return (float)(((mStopTime - mPausedTime) - mBaseTime)*mSecondsPerCount);
	}
	else
	{
		return (float)(((mCurrTime - mPausedTime) - mBaseTime)*mSecondsPerCount);
	}
}

float GameTimer::DeltaTime() const
{
	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (mStopped)
	{
		mPausedTime += (startTime - mStopTime);

		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
}

void GameTimer::Stop()
{
	if (!mStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mStopTime = currTime;
		mStopped = true;
	}
}

void GameTimer::Tick()
{
	if (mStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	mPrevTime = mCurrTime;

	// 如果处理器处于节电模式或更换处理器,mDeltaTime可能为负数
	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}
