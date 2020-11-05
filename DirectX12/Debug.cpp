#include "stdafx.h"
#include "Debug.h"

void Debug::init()
{
	//ファイル展開(存在しない場合作成)
	ofs_.open("Output/ErrorLog.txt");

	//エラーの数を初期化
	errorcnt_ = 0;
}

void Debug::destroy()
{
	//エラーがあった場合
	if (errorcnt_ == 0)
	{
		ofs_ << "エラーなし";
	}
	else
	{
		ofs_ << "ERROR : " << errorcnt_;
	}

	//バッファを出力
	ofs_.close();
}

void Debug::addErrorLog(std::string ErrorMessage)
{
	//エラーの数をカウントしながらバッファに蓄積
	errorcnt_++;
	ofs_ << "ERROR" << errorcnt_ << " : " << ErrorMessage;
}
