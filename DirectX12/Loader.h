#pragma once
namespace loader
{

	//í∏ì_ä÷åW
	class PMDVertex
	{
	private:
		XMFLOAT3 position_;
		XMFLOAT3 normal_;
		XMFLOAT2 uv_;
		std::array<uint16_t, 2> bonenum_;
		uint8_t boneweight_;
		uint8_t edgeflag_;
		friend class PMDFile;

		void load(std::istream& is);

	public:
		PMDVertex() :position_(), normal_(), uv_(), bonenum_(), boneweight_(), edgeflag_() {}

		//get
		inline XMFLOAT3 getPosition()const { return position_; }
		inline XMFLOAT3 getNormal()const { return normal_; }
		inline XMFLOAT2 getUV()const { return uv_; }
		inline uint16_t getBoneIndex(const int Index)const { return bonenum_[Index]; }
		inline float getBoneWeight(const int Index)const { return (Index == 0 ? boneweight_ : (100 - boneweight_)) / 100.0F; }
		inline uint8_t getEdgeFlag()const { return edgeflag_; }

	};

	//É}ÉeÉäÉAÉãä÷åW
	class PMDMaterial
	{
	private:
		XMFLOAT3 diffuse_;
		XMFLOAT3 specular_;
		XMFLOAT3 ambient_;
		float alpha_;
		float shininess_;
		uint8_t toonID_;
		uint8_t edgeflag_;
		uint32_t polygonnumber_;
		std::filesystem::path texturefilepath_;

		void load(std::istream& is);
		friend class PMDFile;

	public:

		//get
		inline XMFLOAT3 getDiffuseColor()const { return diffuse_; }
		inline XMFLOAT3 getSpecularColor()const { return specular_; }
		inline XMFLOAT3 getAmbientColor()const { return ambient_; }
		inline float getAlpha()const { return alpha_; }
		inline float getShininess()const { return shininess_; }
		uint32_t getNumberOfPolygon()const { return polygonnumber_; }
		uint8_t getEdgeFlag()const { return edgeflag_; }
		const std::filesystem::path& getTexturePath()const { return texturefilepath_; }
	};

	//É{Å[Éìä÷åW
	class PMDBone
	{
	private:
		std::string name_;
		uint16_t parent_;
		uint16_t child_;
		uint8_t type_;
		uint16_t targetbone_;
		XMFLOAT3 position_;

		void load(std::istream& is);
		friend class PMDFile;

	public:

		//get
		inline const std::string& getName()const { return name_; }
		inline uint16_t getParent()const { return parent_; }
		inline uint16_t getTarget()const { return targetbone_; }
		inline XMFLOAT3 getPosition()const { return position_; }
	};

	class PMDIk
	{
	private:
		uint16_t boneindex_;
		uint16_t bonetarget_;
		uint16_t iterationnum_;
		uint8_t chainnum_;
		float anglelimit_;
		std::vector<uint16_t> ikbones_;

		void load(std::istream& is);
		friend class PMDFile;

	public:

		//get
		inline uint16_t getTargetBoneID()const { return bonetarget_; }
		inline uint16_t getBoneEff()const { return boneindex_; }
		inline uint16_t getIterations()const { return iterationnum_; }
		inline std::vector<uint16_t> getChains()const { return ikbones_; }
		inline float getAngleLimit()const { return anglelimit_; }
	};


	//ÉtÉFÉCÉXä÷åW
	class PMDFace
	{
		enum FaceType
		{
			kBase,
			kEyebrow,
			kEye,
			kLip,
			kOther,
		};
	private:
		std::string name_;
		uint32_t verticesnum_;
		FaceType facetype_;
		std::vector<uint32_t> faceindices_;
		std::vector<XMFLOAT3> facevertices_;

		void load(std::istream& is);
		friend class PMDFile;

	public:
		inline std::string getName()const { return name_; }
		inline FaceType getType()const { return facetype_; }
		inline uint32_t getVertexCount()const { return uint32_t(facevertices_.size()); }
		inline uint32_t getIndexCount()const { return uint32_t(faceindices_.size()); }
		inline const XMFLOAT3* getFaceVertices()const { return facevertices_.data(); }
		inline const uint32_t* getFacceIndices()const { return faceindices_.data(); }
	};

	class PMDRigitParam
	{
		enum ShapeType
		{
			kShape_SPHERE,
			kShape_BOX,
			kShape_CAPSULE,
		};

		enum RigidBodyType
		{
			kRigid_Body_BONE,			//É{Å[Éìí«è]
			kRigid_Body_PHYSICS,		//ï®óùââéZ
			kRigid_Body_PHYSICS_BONE_CORRECT,	//ï®óùââéZ(É{Å[Éìà íuçáÇÌÇπ)
		};
	private:

		void load(std::istream& is);

		std::string name_;
		uint16_t boneid_;
		uint8_t groupid_;
		uint16_t groupmask_;
		ShapeType shapetype_;
		RigidBodyType bodytype_;
		XMFLOAT3 position_;
		XMFLOAT3 rotation_;
		float shapeW_;
		float shapeH_;
		float shapeD_;
		float weight_;
		float attenuationpos_;
		float attenuationrot_;
		float recoil_;
		float friction_;
		friend class PMDFile;

	};

	//ä÷êﬂä÷åW
	class PMDJointParam 
	{
	private:

		std::string name_;
		std::array<uint32_t, 2> targetrigidbodies_;
		std::array<XMFLOAT3, 2> constraintpos_;
		std::array<XMFLOAT3, 2> constraintrot_;
		XMFLOAT3 position_;
		XMFLOAT3 rotation_;
		XMFLOAT3 springpos_;
		XMFLOAT3 springrot_;

		void load(std::istream& is);
		friend class PMDFile;

	};

	class PMDFile
	{
	private:
		float version_;
		std::string name_;
		std::string comment_;

		std::vector<PMDVertex> vertices_;
		std::vector<uint16_t> indices_;
		std::vector<PMDMaterial> materials_;
		std::vector<PMDBone> bones_;
		std::vector<PMDIk> iks_;
		std::vector<PMDFace> faces_;

		std::vector<std::string> toontexture_;
		std::vector<PMDRigitParam> rigidbodies_;
		std::vector<PMDJointParam> joints_;
	public:
		PMDFile() {}
		PMDFile(std::istream& Is);

		//get
		inline const std::string& getName()const { return name_; }
		inline const std::string& getComment()const { return comment_; }

		inline uint32_t getVertexCount()const { return uint32_t(vertices_.size()); }
		inline uint32_t getIndexCount()const { return uint32_t(indices_.size()); }
		inline uint32_t getMaterialCount()const { return uint32_t(materials_.size()); }
		inline uint32_t getBoneCount()const { return uint32_t(bones_.size()); }
		inline uint32_t getIkCount()const { return uint32_t(iks_.size()); }
		inline uint32_t getFaceCount()const { return uint32_t(faces_.size()); }
		inline uint32_t getRigidBodyCount()const { return rigidbodies_.size(); }
		inline uint32_t getJointCount()const { return joints_.size(); }
		inline const PMDVertex& getVertex(const int Index)const { return vertices_[Index]; }
		inline const PMDVertex* getVertex()const { return vertices_.data(); }
		inline uint32_t getIndices(const int Index)const { return indices_[Index]; }
		inline const uint16_t* getIndices()const { return indices_.data(); }
		const PMDMaterial& getMaterial(const int Index)const { return materials_[Index]; }
		const PMDBone& getBone(const int Index)const { return bones_[Index]; }
		const PMDIk& getIk(const int Index)const { return iks_[Index]; }
		const PMDFace& getFace(const int Index)const { return faces_[Index]; }
		const PMDFace& getFaceBase()const { auto itr = std::find_if(faces_.begin(), faces_.end(), [](const auto& v) {return v.getType() == PMDFace::kBase; }); return *itr; }
	};

	//ÉmÅ[Éhä÷åW
	class VMDNode
	{
	private:
		uint32_t keyframe_;
		XMFLOAT3 location_;
		XMFLOAT4 rotation_;
		std::string name_;
		uint8_t interpolation_[64];

		void load(std::istream& is);
		friend class PMDFile;

	public:
		inline const std::string& getName()const { return name_; }
		inline uint32_t getKeyFrameNumber()const { return keyframe_; }
		inline const XMFLOAT3& getLocation()const { return location_; }
		inline const XMFLOAT4& getRotation()const { return rotation_; }
		XMFLOAT4 getBezierParam(const int Index)const;
	};

	//ïœå`ä÷åW
	class VMDMorph
	{
	private:
		std::string name_;
		uint32_t keyframe_;
		float weight_;

		void load(std::istream& is);

	public:
		inline const std::string& getName()const { return name_; }
		inline uint32_t getKeyFrameNumber()const { return keyframe_; }
		inline float getWeight()const { return weight_; }
	};

	//VMDÉtÉ@ÉCÉã
	class VMDFile
	{
	private:
		uint32_t keyframecount_;
		std::map < std::string, std::vector<VMDNode>> animationmap_;
		std::vector<std::string> nodenamelist_;
		std::map<std::string, std::vector<VMDMorph>> morphmap_;
		std::vector<std::string> morphnamelist_;

		const std::vector<VMDNode>* findNode(const std::string& NodeName)
		{
			auto it = animationmap_.find(NodeName);
			if (it != animationmap_.end())
				return &(it->second);

			return nullptr;
		}
	public:
		VMDFile() = default;
		VMDFile(std::istream& Is);

		uint32_t getNodeCount()const { return uint32_t(nodenamelist_.size()); }
		const std::string& getNodeName(int Index)const { return nodenamelist_[Index]; }
		uint32_t getMorphCount()const { return uint32_t(morphnamelist_.size()); }
		const std::string& getMorphName(int Index)const { return morphnamelist_[Index]; }
		std::vector<VMDNode> getKeyFrames(const std::string& Nodename) { return animationmap_[Nodename]; }
		std::vector<VMDMorph> getMorphKeyFrames(const std::string& MorphName) { return morphmap_[MorphName]; }
		uint32_t getKeyFrameCount()const { return keyframecount_; }
	};

	struct Memorybuf :std::streambuf
	{
		Memorybuf(char* Base, size_t Size) :beg(Base), end(Base + Size)
		{
			this->setg(beg, beg, end);
		}

		virtual pos_type seekoff(off_type Off, std::ios_base::seekdir Dir, std::ios_base::openmode Which = std::ios_base::in)override
		{
			if (Dir == std::ios_base::cur)
			{
				gbump(int(Off));
			}
			else if (Dir == std::ios_base::end)
			{
				setg(beg, end + Off, end);
			}
			else if (Dir == std::ios_base::beg)
			{
				setg(beg, beg + Off, end);
			}
			return gptr() - eback();
		}

		virtual pos_type seekpos(std::streampos Pos, std::ios_base::openmode Mode)override
		{
			return seekoff(Pos - pos_type(off_type(0)), std::ios_base::beg, Mode);
		}

		char* beg;
		char* end;
	};

	struct MemoryStream :virtual Memorybuf, std::istream
	{
		MemoryStream(char* Mem, size_t Size) :Memorybuf(Mem, Size), std::istream(static_cast<std::streambuf*>(this))
		{
		}
	};

}