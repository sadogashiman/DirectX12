#pragma once
#include "ModelData.h"
#include "DescriptorManager.h"

struct MaterialData
{
	XMFLOAT4 diffuse;
	XMFLOAT4 ambient;
	XMFLOAT4 specular;
	UINT usetexture;
	UINT edgeflag;
};

struct Resource
{
	ComPtr<ID3D12Resource1> resource;
	DescriptorHandle descriptor;
};

class Material
{
private:
	MaterialData materialparam_;
	Resource texture_;
	Resource materialcbuffer_;
public:
	Material(const MaterialData& Param) :materialparam_(Param) {}
	void update();

	//get
	inline XMFLOAT4 getDiffuseColor()const { return materialparam_.diffuse; }
	inline XMFLOAT4 getAmbientColor()const { return materialparam_.ambient; }
	inline XMFLOAT4 getSpexularColor()const { return materialparam_.specular; }
	inline Resource getConstantBuffer()const { return materialcbuffer_; }
	inline DescriptorHandle getTextureDescriptor()const { return texture_.descriptor; }
	inline bool getEdgeFlag()const { return materialparam_.edgeflag != 0; }

	//set
	inline void setTexture(Resource& Resource) { texture_ = Resource; }
	inline void setConstantBuffer(Resource& Resource) { materialcbuffer_ = Resource; }

	//has
	inline bool hasTexture()const { return materialparam_.usetexture != 0; }
};

class Bone
{
private:
	std::vector<Bone*> child_;
	std::string name_;
	Bone* parent_;
	XMVECTOR translation_;
	XMVECTOR rotation_;
	XMVECTOR initialtranslation_;
	XMMATRIX localmatrix_;
	XMMATRIX worldmatrix_;
	XMMATRIX invbindmatrix_;

	inline void addChild(Bone* Bone) { child_.push_back(Bone); }

public:
	Bone();
	Bone(const std::string& Name);
	
	inline void updateLocalMatrix() { localmatrix_ = XMMatrixRotationQuaternion(rotation_) * XMMatrixTranslationFromVector(translation_); }
	inline void updateWorldMatrix()
	{
		updateLocalMatrix();
		auto parentmatrix = XMMatrixIdentity();
		if (parent_)
		{
			parentmatrix = parent_->getWorldMatrix();
		}
		worldmatrix_ = localmatrix_ * parentmatrix;
	}
	inline void updateMatrices()
	{
		updateWorldMatrix();
		for (auto c : child_)
		{
			c->updateMatrices();
		}
	}

	//get
	inline XMVECTOR getTranslation()const { return translation_; }
	inline XMVECTOR getRotation()const { return rotation_; }
	inline XMMATRIX getLocalMatrix()const { return localmatrix_; }
	inline XMMATRIX getWorldMatrix()const { return worldmatrix_; }
	inline XMMATRIX getInvBindMatrix()const { return invbindmatrix_; }
	inline XMVECTOR getInitialTranslation()const { return initialtranslation_; }
	inline const std::string& getName()const { return name_; }
	inline Bone* getParent()const { return parent_; }

	//set
	inline void setTranslation(const XMFLOAT3& Trans) { translation_ = XMLoadFloat3(&Trans); }
	inline void setTranslation(const XMVECTOR Trans) { translation_ = Trans; }
	inline void setRotation(const XMFLOAT4& Rot) { rotation_ = XMLoadFloat4(&Rot); }
	inline void setRotation(const XMVECTOR Rot) { rotation_ = Rot; }
	inline void setInitialTranslation(const XMFLOAT3& Trans) { initialtranslation_ = XMLoadFloat3(&Trans); }
	inline void setInvBindMatrix(const XMMATRIX& InvBindMatrix) { invbindmatrix_ = InvBindMatrix; }
	inline void setParent(Bone* Parent)
	{
		parent_ = Parent;

		if (parent_ != nullptr)
		{
			parent_->addChild(this);
		}
	}
};

class PMDBoneIK
{
private:
	Bone* effector_;
	Bone* target_;
	std::vector<Bone*> ikchains_;
	float anglelimit_;
	int iteration_;
public:
	PMDBoneIK() :effector_(nullptr), target_(nullptr), anglelimit_(0.0F), iteration_(0) {}
	PMDBoneIK(Bone* Target, Bone* Eff) :effector_(Eff), target_(Target) {}

	//get
	inline Bone* getEffector()const { return effector_; }
	inline Bone* getTarget()const { return target_; }
	float getAngleWeight()const { return anglelimit_; }
	const std::vector<Bone*>& getChains()const { return ikchains_; }
	int getIterationCount()const { return iteration_; }

	//set
	inline void setAngleLimit(float Angle) { anglelimit_ = Angle; }
	inline void setIterationCount(int IteraionCount) { iteration_ = IteraionCount; }
	inline void setIKChains(std::vector<Bone*>& Chains) { ikchains_ = Chains; }
};


class Model
{
public:
	struct PMDVertex
	{
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
		XMUINT2 boneindices;
		XMFLOAT2 boneweight;
		UINT edgeflag;
	};

	struct SceneParameter
	{
		XMFLOAT4X4 view;
		XMFLOAT4X4 projection;
		XMFLOAT4 lightDirection;
		XMFLOAT4 eyePosition;
		XMFLOAT4 outlineColor;
		XMFLOAT4X4 lightviewproj;
		XMFLOAT4X4 lightviewprojbias;
	};
	struct BoneParameter
	{
		XMFLOAT4X4 bone[512];
	};

private:
	SceneParameter sceneparam_;
	BoneParameter boonematerices_;
	std::vector<PMDVertex> hostmemvertices_;
	std::vector<Material> material_;

	struct Mesh
	{
		uint32_t indexoffset;
		uint32_t indexcount;
	};

	std::vector<Mesh> meshes_;
	std::vector<Bone*> bones_;

	ComPtr<ID3D12RootSignature> rootsignature_;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> pipelinestate_;
	ComPtr<ID3D12GraphicsCommandList> bundlenormaldraw_;
	ComPtr<ID3D12GraphicsCommandList> bundleoutline_;
	ComPtr<ID3D12GraphicsCommandList> bundleshadow_;

	std::vector<std::vector<ComPtr<ID3D12GraphicsCommandList>>> commandshadow_;

	ComPtr<ID3D12Resource1> indexbuffer_;
	UINT indexbuffersize_;
	std::vector<ComPtr<ID3D12Resource1>> vertexbuffer_;
	std::vector<ComPtr<ID3D12Resource1>> sceneparametercb_;
	std::vector<ComPtr<ID3D12Resource1>> boneparametercb_;
	ComPtr<ID3D12Resource1> texturedummy_;

	DescriptorHandle shadowmap_;
	DescriptorHandle dummytexdescriptor_;

	//表情モーフベース頂点情報
	struct PMDFaceBaseInfo
	{
		std::vector<uint32_t> indices;
		std::vector<XMFLOAT3> verticespos;
	};
	PMDFaceBaseInfo facebaseinfo_;

	//表情モーフオフセット頂点情報
	struct PMDFaceInfo
	{
		std::string name;
		std::vector<uint32_t> indices;
		std::vector<XMFLOAT3> verticesoffset;
	};

	std::vector<PMDFaceInfo> faceoffsetinfo_;
	std::vector<float> facemophweights_;
	std::vector<PMDBoneIK> boneIKlist_;

	void initRootSignature();
	void initPipelineState();
	void initConstantBuffer();
	void initBundles();
	void initDummyTexture();
	void computeMorph();
public:
	void init(const char* Filename);
	void destroy();

	//set
	inline void setSceneParameter(const SceneParameter& Param) { sceneparam_ = Param; }
	inline void setShadowMap(DescriptorHandle Handle) { shadowmap_ = Handle; }

	void updateMatrices();
	void update(uint32_t ImageIndex);

	void draw(ComPtr<ID3D12GraphicsCommandList> CommandList);
	void drawShadow(ComPtr<ID3D12GraphicsCommandList> Commandlist);

	//ボーン情報
	inline uint32_t getBoneCount()const { return uint32_t(bones_.size()); }
	inline const Bone* getBone(const int Index)const { return bones_[Index]; }
	inline Bone* getBone(const int Index) { return bones_[Index]; }

	inline uint32_t getFaceMorphCount()const { return uint32_t(faceoffsetinfo_.size()); }
	const Bone* getFaceMorphIndex(const std::string& FaceName)const;
	void setFaceMorphWeight(const int Index, const float Weight);

	inline uint32_t getBoneIKCount()const { return uint32_t(boneIKlist_.size()); }
	inline const PMDBoneIK& getBoneIK(const int Index)const { return boneIKlist_[Index]; }

};

