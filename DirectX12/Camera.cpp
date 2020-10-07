#include "stdafx.h"
#include "Camera.h"
#include "Singleton.h"
#include "DirectInput.h"
//C言語のmax・minマクロの定義を解除
#undef max
#undef min

Camera::Camera()
{
	ZeroMemory(this, sizeof(Camera));
	yaw_ = XM_PI;
	lookdirection_ = XMFLOAT3(0.0F, 0.0F, -1.0);
	updirection_ = XMFLOAT3(0.0F, 1.0F, 0.0);
	movespeed_ = 20.0F;
	turnspeed_ = XM_PIDIV2;
	position_ = initialposition_ = XMFLOAT3(0.0F, 0.0F, 0.0F);
}

Camera::~Camera()
{
}

void Camera::init(XMFLOAT3 Position)
{
	initialposition_ = Position;
	reset();
}

void Camera::update(const float ElapsedSecond)
{
	XMFLOAT3 move;
	auto input = Singleton<DirectInput>::getPtr();

	if (input->isKeyState(DIK_A))
		move.x = -1.0F;
	if (input->isKeyState(DIK_D))
		move.x = 1.0F;
	if (input->isKeyState(DIK_W))
		move.z = -1.0F;
	if (input->isKeyState(DIK_S))
		move.z = 1.0F;

	if (std::abs(move.x) > 0.1F && std::abs(move.z) > 0.1F)
	{
		XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
		move.x = XMVectorGetX(vector);
		move.z = XMVectorGetZ(vector);
	}

	float moveinterval = movespeed_ * ElapsedSecond;
	float rotateinterval = turnspeed_ * ElapsedSecond;

	if (input->isKeyState(DIK_LEFT))
		yaw_ += rotateinterval;
	if (input->isKeyState(DIK_RIGHT))
		yaw_ -= rotateinterval;
	if (input->isKeyState(DIK_UP))
		pitch_ += rotateinterval;
	if (input->isKeyState(DIK_DOWN))
		pitch_ -= rotateinterval;

	pitch_ = std::min(pitch_, XM_PIDIV4);
	pitch_ = std::max(-XM_PIDIV4, pitch_);

	//カメラ移動
	float x = move.x * std::cos(yaw_) - move.z * std::sin(yaw_);
	float z = move.x * std::sin(yaw_) - move.z * std::cos(yaw_);
	position_.x += x * moveinterval;
	position_.z += z * moveinterval;

	float r = std::cos(pitch_);
	lookdirection_.x = r * std::sin(yaw_);
	lookdirection_.y = std::sin(pitch_);
	lookdirection_.z = r * std::cos(yaw_);
}

void Camera::reset()
{
	position_ = initialposition_;
	yaw_ = XM_PI;
	pitch_ = 0.0F;
	lookdirection_ = { 0,0,-1 };
}