#include "stdafx.h"
#include "Timer.h"
#include "Support.h"

Timer::Timer()
{
	ZeroMemory(this, sizeof(Timer));
	running_ = false;
	stopwatch_ = false;
	prevtime_ = std::chrono::high_resolution_clock::now();
}

Timer::~Timer()
{
}

void Timer::update()
{
	//稼働状態か確認
	if (running_)
	{
		//現在時間を取得
		nowtime_ = std::chrono::high_resolution_clock::now();

		if (stopwatch_)
		{
			//経過時間を算出
			remaining_ = std::chrono::duration_cast<std::chrono::milliseconds>(nowtime_ - starttime_);
		}
	}
}

void Timer::setTimerStatus(const bool RuningFrag)
{
	//フラグをメンバにコピー
	running_ = RuningFrag;
}

void Timer::startTimer()
{
	//計測開始時間を取得
	starttime_ = std::chrono::high_resolution_clock::now();

	//ストップウォッチ機能を有効にする
	stopwatch_ = true;
}

void Timer::stopTimer()
{
	//計測終了時間を取得
	endtime_ = std::chrono::high_resolution_clock::now();

	//ストップウォッチ機能を無効にする
	stopwatch_ = false;

	//ストップウォッチ機能が有効だった時間を算出
	runningtime_ = std::chrono::duration_cast<std::chrono::milliseconds>(endtime_ - starttime_);
}

bool Timer::fpsControl(FrameRate Rate)
{
	//前回との差分を計算
	auto difference = std::chrono::duration_cast<std::chrono::microseconds>((nowtime_ - prevtime_));
	//16ミリ秒経過していたら更新を許可する
	if (difference.count() > Rate)
	{
		prevtime_ = nowtime_;
		return true;
	}

	return false;
}

void Timer::destroy()
{
	//昨日を停止
	running_ = false;
	stopwatch_ = false;
}
