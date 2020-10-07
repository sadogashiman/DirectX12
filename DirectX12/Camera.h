#pragma once
class Camera
{
private:
	void reset();

	XMFLOAT3 initialposition_;
	XMFLOAT3 position_;
	float yaw_;
	float pitch_;
	XMFLOAT3 lookdirection_;
	XMFLOAT3 updirection_;
	float movespeed_;
	float turnspeed_;
public:
	Camera();
	~Camera();
	void init(XMFLOAT3 Position);
	void update(const float ElapsedSecond);

	//set
	inline void setMoveSpeed(const float UnitsPerSecond) {movespeed_ = UnitsPerSecond;}
	inline void setTurnSpeed(const float RadiansPerSecond) { turnspeed_ = RadiansPerSecond; };

	//get
	XMMATRIX getViewMatrix()const;
	inline XMMATRIX getProjectionMatrix(const float Fov, const float AspectRatio, const float NearPlane, const float FarPlane)const { return XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearPlane, FarPlane); }

};

