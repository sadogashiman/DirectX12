#include "Loader.h"
#include <pshpack1.h>

namespace loader
{
	namespace rawblock
	{
		struct PMDHeader
		{
			unsigned char magic[3];
			float version;
			char name[20];
			char comment[256];
		};
		struct PMDVertex
		{
			XMFLOAT3 position;
			XMFLOAT3 normal;
			XMFLOAT2 uv;
			uint16_t boneID[2];
			uint8_t boneweight;
			uint8_t noedgeflag;
		};
		struct PMDMaterial
		{
			XMFLOAT3 deffuse;
			float alpha;
			float shininess;
			XMFLOAT3 specular;
			XMFLOAT3 ambient;
			uint8_t toonID;
			uint8_t edgeflag;
			uint32_t polygonnumber;
			std::filesystem::path texturepath;
		};
		struct PMDBone
		{
			char name[20];
			uint16_t parentboneID;
			uint16_t childboneID;
			uint8_t type;
			uint16_t targetboneID;
			XMFLOAT3 position;
		};
		struct PMDIk
		{
			uint16_t destboneID;
			uint16_t childboneID;
			uint8_t chainnum;
			uint16_t interationnum;
			float anglelimit;
		};

		struct VMDHeader
		{
			UCHAR magic[30];
			char modelname[20];
		};

		float readFloat(std::istream& Is)
		{
			union { float f; char bin[4]; }data;
			Is.read(data.bin, 4);
			return data.f;
		}

		uint8_t readUint8(std::istream& Is) { char v; Is.read(&v, 1); return uint8_t(v); }
		uint16_t readUint16(std::istream& Is)
		{
			union { uint16_t u16; char bin[2]; }data;
			Is.read(data.bin, 2);
			return data.u16;
		}

		uint32_t readUint32(std::istream& Is)
		{
			union { uint32_t u32; char bin[4]; }data;
			Is.read(data.bin, 4);
			return data.u32;
		}

		XMFLOAT2 readFloat2(std::istream& Is) { XMFLOAT2 v; v.x = readFloat(Is); v.y = readFloat(Is); return v; }
		XMFLOAT3 readFloat3(std::istream& Is) { XMFLOAT3 v; v.x = readFloat(Is); v.y = readFloat(Is); v.z = readFloat(Is); return v; }
		XMFLOAT4 readFloat4(std::istream& Is) { XMFLOAT4 v; v.x = readFloat(Is); v.y = readFloat(Is); v.z = readFloat(Is); v.w = readFloat(Is); return v; }

#ifdef USE_LEFTHAND
		XMFLOAT3 flipToRH(XMFLOAT3 V) { V.z *= -1.0F; return V; }
		XMFLOAT4 flipToRH(XMFLOAT4 V) { V.z *= -1.0F; V.w *= -1.0F; return V; }

#else
		XMFLOAT3 flipToRH(XMFLOAT3 V) { return V; }
		XMFLOAT4 flipToRH(XMFLOAT4 V) { return V; }

#endif // USE_LEFTHAND
	}
}

#include <poppack.h>

namespace loader
{
	std::istream& loadFloatFromHex(std::istream& Stream, float& V)
	{
		return Stream.read(reinterpret_cast<char*>(&V), sizeof(V));
	}

	std::istream& operator>>(std::istream& Stream, XMFLOAT2& V)
	{
		loadFloatFromHex(Stream, V.x);
		loadFloatFromHex(Stream, V.y);
		return Stream;
	}

	std::istream& operator>>(std::istream& Stream, XMFLOAT3& V)
	{
		Stream >> V.x >> V.y >> V.z;
		return Stream;
	}

	std::istream& operator>> (std::istream& Stream, rawblock::PMDVertex& V)
	{
		Stream >> V.position >> V.normal >> V.uv >> V.boneID[0] >> V.boneID[1] >> V.boneweight >> V.noedgeflag;
		return Stream;
	}

	void PMDVertex::load(std::istream& is)
	{
		position_ = rawblock::flipToRH(rawblock::readFloat3(is));
		normal_ = rawblock::flipToRH(rawblock::readFloat3(is));
		uv_ = rawblock::readFloat2(is);
		bonenum_[0] = rawblock::readUint16(is);
		bonenum_[1] = rawblock::readUint16(is);
		boneweight_ = rawblock::readUint8(is);
		edgeflag_ = rawblock::readUint8(is);
	}

	void PMDMaterial::load(std::istream& is)
	{
		diffuse_ = rawblock::flipToRH(rawblock::readFloat3(is));
		alpha_ = rawblock::readFloat(is);
		shininess_ = rawblock::readFloat(is);
		specular_ = rawblock::readFloat3(is);
		ambient_ = rawblock::readFloat3(is);
		toonID_ = rawblock::readUint8(is);
		edgeflag_ = rawblock::readUint8(is);
		polygonnumber_ = rawblock::readUint32(is);

		char buf[20];
		is.read(buf, sizeof(buf));
		texturefilepath_ = buf;
	}

	void PMDBone::load(std::istream& is)
	{
		char buf[20];
		is.read(buf, sizeof(buf));
		name_ = buf;
		parent_ = rawblock::readUint16(is);
		child_ = rawblock::readUint16(is);
		type_ = rawblock::readUint8(is);
		targetbone_ = rawblock::readUint16(is);
		position_ = rawblock::flipToRH(rawblock::readFloat3(is));
	}

	void PMDIk::load(std::istream& is)
	{
		boneindex_ = rawblock::readUint16(is);
		bonetarget_ = rawblock::readUint16(is);
		chainnum_ = rawblock::readUint8(is);
		iterationnum_ = rawblock::readUint16(is);
		anglelimit_ = rawblock::readFloat(is);

		ikbones_.reserve(chainnum_);
		for (uint32_t i = 0; i < chainnum_; ++i)
		{
			ikbones_.emplace_back(rawblock::readUint16(is));
		}
	}

	void PMDFace::load(std::istream& is)
	{
		char buf[20];
		is.read(buf, sizeof(buf));
		name_ = buf;
		verticesnum_ = rawblock::readUint32(is);
		facetype_ = FaceType(rawblock::readUint8(is));
		facevertices_.reserve(verticesnum_);
		faceindices_.reserve(verticesnum_);

		for (uint32_t i = 0; i < verticesnum_; ++i)
		{
			faceindices_.emplace_back(rawblock::readUint32(is));
			facevertices_.emplace_back(rawblock::flipToRH(rawblock::readFloat3(is)));
		}
	}

	void PMDRigitParam::load(std::istream& is)
	{
		char buf[20];
		is.read(buf, sizeof(buf));
		name_ = buf;

		boneid_ = rawblock::readUint16(is);
		groupid_ = rawblock::readUint8(is);
		groupmask_ = rawblock::readUint16(is);
		shapeW_ = rawblock::readFloat(is);
		shapeH_ = rawblock::readFloat(is);
		shapeD_ = rawblock::readFloat(is);
		position_ = rawblock::readFloat3(is);
		rotation_ = rawblock::readFloat3(is);
		weight_ = rawblock::readFloat(is);
		attenuationpos_ = rawblock::readFloat(is);
		attenuationrot_ = rawblock::readFloat(is);
		recoil_ = rawblock::readFloat(is);
		friction_ = rawblock::readFloat(is);
		bodytype_ = RigidBodyType(rawblock::readUint8(is));
	}

	void PMDJointParam::load(std::istream& is)
	{
		char buf[20];
		is.read(buf, sizeof(buf));
		name_ = buf;

		for (auto& v : targetrigidbodies_)
		{
			v = rawblock::readUint32(is);
		}

		position_ = rawblock::readFloat3(is);
		rotation_ = rawblock::readFloat3(is);

		for (auto& v : constraintpos_)
		{
			v = rawblock::readFloat3(is);
		}
		for (auto& v : constraintrot_)
		{
			v = rawblock::readFloat3(is);
		}
		springpos_ = rawblock::readFloat3(is);
		springrot_ = rawblock::readFloat3(is);
	}
	PMDFile::PMDFile(std::istream& Is)
	{
		rawblock::PMDHeader header;
		Is.read(reinterpret_cast<char*>(&header), sizeof(header));

		version_ = header.version;
		name_ = header.name;
		comment_ = header.comment;

		//頂点の読み込み
		auto vertexcnt = rawblock::readUint32(Is);
		vertices_.resize(vertexcnt);
		std::for_each(vertices_.begin(), vertices_.end(), [&](auto& v) {v.load(Is); });

		auto indexcnt = rawblock::readUint32(Is);
		indices_.reserve(indexcnt);
		auto polygoncnt = indexcnt / 3;
		for (uint32_t i = 0; i < polygoncnt; ++i)
		{
			auto idx0 = rawblock::readUint16(Is);
#ifdef USE_LEFTHAND
			auto idx2 = rawblock::readUint16(Is);
			auto idx1 = rawblock::readUint16(Is);
#else
			auto idx1 = rawblock::readUint16(Is);
			auto idx2 = rawblock::readUint16(Is);
#endif // USE_LEFTHAND

			indices_.push_back(idx0);
			indices_.push_back(idx1);
			indices_.push_back(idx2);
		}

		//マテリアルのカウント
		auto materialcnt = rawblock::readUint32(Is);
		materials_.resize(materialcnt);
		std::for_each(materials_.begin(), materials_.end(), [&](auto& v) {v.load(Is); });

		//ボーンのカウント
		auto bonecnt = rawblock::readUint16(Is);
		bones_.resize(bonecnt);
		std::for_each(bones_.begin(), bones_.end(), [&](auto& v) {v.load(Is); });

		//インバースキネマティクスリストのカウント
		auto iklistcnt = rawblock::readUint16(Is);
		iks_.resize(iklistcnt);
		std::for_each(iks_.begin(), iks_.end(), [&](auto& v) {v.load(Is); });

		//フェイスのカウント
		auto facecnt = rawblock::readUint16(Is);
		faces_.resize(facecnt);
		std::for_each(faces_.begin(), faces_.end(), [&](auto& v) {v.load(Is); });

		//表情枠
		auto facedispcnt = rawblock::readUint8(Is);
		auto facedispblockbytes = facedispcnt * sizeof(uint16_t);
		Is.seekg(facedispblockbytes, std::ios::cur);

		//ボーン枠の名前
		auto bonedispnamecnt = rawblock::readUint8(Is);
		auto bonedispnameblockbytes = bonedispnamecnt * sizeof(char[50]);
		Is.seekg(bonedispnameblockbytes, std::ios::cur);

		//ボーン枠
		auto bonedispcnt = rawblock::readUint8(Is);
		auto bonedispblockbytes = bonedispcnt * sizeof(char[3]);
		Is.seekg(bonedispblockbytes, std::ios::cur);

		//英語名ヘッダ
		auto engnamecnt = rawblock::readUint32(Is);
		auto engnameblockbytes = engnamecnt * sizeof(char[20 + 256]);
		Is.seekg(engnameblockbytes, std::ios::cur);

		//英語名ボーン
		auto engboneblockbytes = bones_.size() * sizeof(char[20]);
		Is.seekg(engboneblockbytes, std::ios::cur);

		//英語名表情リスト


	}

	template<class T>
	bool keyFrameComparer(const T& A, const T& B)
	{
		return A.getKeyFrameNumber() < B.getKeyFrameumber();
	}


	VMDFile::VMDFile(std::istream& Is)
	{
	}
	void VMDNode::load(std::istream& is)
	{
		char namebuf[15];
		is.read(namebuf, sizeof(namebuf));
		name_ = namebuf;
		keyframe_ = rawblock::readUint32(is);
		location_ = rawblock::flipToRH(rawblock::readFloat3(is));
		rotation_ = rawblock::flipToRH(rawblock::readFloat4(is));

		is.read(reinterpret_cast<char*>(interpolation_), sizeof(interpolation_));
	}
	XMFLOAT4 VMDNode::getBezierParam(const int Index) const
	{
		XMFLOAT4 ret;
		ret.x = float(interpolation_[4 * 0 + Index]) / 127.0F;
		ret.y = float(interpolation_[4 * 1 + Index]) / 127.0F;
		ret.z = float(interpolation_[4 * 2 + Index]) / 127.0F;
		ret.w = float(interpolation_[4 * 3 + Index]) / 127.0F;

		return ret;
	}
	void VMDMorph::load(std::istream& is)
	{
		char namebuf[15];
		is.read(namebuf, sizeof(namebuf));
		name_ = namebuf;
		keyframe_ = rawblock::readUint32(is);
		weight_ = rawblock::readFloat(is);
	}


}

