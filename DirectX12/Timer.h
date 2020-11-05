#pragma once
enum FrameRate
{
	k60Hz = 16666,
	k144Hz = 6944,
	k244Hz = 4098,
};

class Timer
{
private:
	bool running_;												 //稼働状態かを判定するフラグ
	bool stopwatch_;											 //時間計測をするかを判定するフラグ
	std::chrono::high_resolution_clock::time_point starttime_;   //計測開始時間
	std::chrono::high_resolution_clock::time_point prevtime_;    //前回時間を図った時の時間
	std::chrono::high_resolution_clock::time_point nowtime_;     //現在時間
	std::chrono::high_resolution_clock::time_point endtime_;	 //計測終了時間
	std::chrono::milliseconds runningtime_;						 //タイマーが稼働していた時間
	std::chrono::milliseconds remaining_;						 //前回計測時との差分
	std::chrono::milliseconds startuptime_;

	//cpuクロック
	float tickperms_;
	float frametime_;
	INT64 frequency_;
	INT64 cpustart_;

public:
	Timer();
	~Timer();
	void update();								//タイマーの更新をする関数(タイマーが稼働状態でないときは何もしない)
	void setTimerStatus(const bool RuningFrag); //タイマーの更新をするかどうかを設定する関数
	void startTimer();							//時間計測を開始する関数
	void stopTimer();							//時間計測を終了する関数
	bool fpsControl(FrameRate Rate = k60Hz);	//フレーム更新をするタイミングでtrueを返す関数
	void destroy();

	//get
	inline const UINT64 getElapsedTime()const { return remaining_.count(); }		//計測開始から関数呼び出し時までの経過時間を返す(ミリ秒)
	inline const UINT64 getRunningTime()const { return runningtime_.count(); }		//計測開始から終了までの経過時間を返す(ミリ秒)
	inline const bool getTimerStatus()const { return running_; }					//タイマーが現在稼働しているかどうかを返す
	inline const bool getStopWatchStatus()const { return stopwatch_; }				//ストップウォッチ機能が有効かどうかを返す関数
	inline const float getCpuTime()const { return frametime_; }
};

