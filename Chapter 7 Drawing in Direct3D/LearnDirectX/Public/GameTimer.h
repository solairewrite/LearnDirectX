#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime() const; // 单位s
	float DeltaTime() const; // 单位s

	void Reset(); // 消息循环开始前执行
	void Start();
	void Stop();
	void Tick();

private:
	double mSecondsPerCount; // s/1次计数
	double mDeltaTime;

	__int64 mBaseTime; // 调用Reset()的时刻
	__int64 mPausedTime; // 游戏暂停的总时长
	__int64 mStopTime; // 如果游戏暂停,暂停开始的时刻
	__int64 mPrevTime; // 上一帧的时刻
	__int64 mCurrTime; // 当前时间

	bool mStopped; // 游戏是否暂停
};

#endif
