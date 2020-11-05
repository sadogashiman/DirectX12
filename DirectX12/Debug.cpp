#include "stdafx.h"
#include "Debug.h"

void Debug::init()
{
	//�t�@�C���W�J(���݂��Ȃ��ꍇ�쐬)
	ofs_.open("Output/ErrorLog.txt");

	//�G���[�̐���������
	errorcnt_ = 0;
}

void Debug::destroy()
{
	//�G���[���������ꍇ
	if (errorcnt_ == 0)
	{
		ofs_ << "�G���[�Ȃ�";
	}
	else
	{
		ofs_ << "ERROR : " << errorcnt_;
	}

	//�o�b�t�@���o��
	ofs_.close();
}

void Debug::addErrorLog(std::string ErrorMessage)
{
	//�G���[�̐����J�E���g���Ȃ���o�b�t�@�ɒ~��
	errorcnt_++;
	ofs_ << "ERROR" << errorcnt_ << " : " << ErrorMessage;
}
