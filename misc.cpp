#include "misc.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#include <boost/regex.hpp>
#pragma GCC diagnostic pop

namespace spn {
	const char* AngleInfo<Degree_t>::name = "degree";
	const char* AngleInfo<Degree_t>::name_short = "deg";
	const char* AngleInfo<Radian_t>::name = "radian";
	const char* AngleInfo<Radian_t>::name_short = "rad";

	float CramerDet(const Vec3& v0, const Vec3& v1, const Vec3& v2) {
		return v0.cross(v1).dot(v2);
	}
	float CramerDet(const Vec2& v0, const Vec2& v1) {
		return v0.x * v1.y - v1.x * v0.y;
	}

	Vec3 CramersRule(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& a0, float detInv) {
		return Vec3(CramerDet(a0,v1,v2) * detInv,
					CramerDet(v0,a0,v2) * detInv,
					CramerDet(v0,v1,a0) * detInv);
	}
	Vec2 CramersRule(const Vec2& v0, const Vec2& v1, const Vec2& a0, float detInv) {
		return Vec2(CramerDet(a0, v1) * detInv,
					CramerDet(v0, a0) * detInv);
	}
	void GramSchmidtOrth(Vec3& v0, Vec3& v1, Vec3& v2) {
		Assert(Throw, v0.normalize() >= 1e-6f)
		v1 -= v0 * v0.dot(v1);
		Assert(Throw, v1.normalize() >= 1e-6f)
		v2 -= v0*v0.dot(v2) + v1*v1.dot(v2);
		Assert(Throw, v2.normalize() >= 1e-6f)
	}

	// ------------ YawPitchDist ------------
	// 原点からのYawとPitch,Distanceを算出。Rollは無視
	YawPitchDist YawPitchDist::FromPos(const spn::Vec3& pos) {
		YawPitchDist ypd;
		// Distance
		spn::Vec3 v(pos);
		ypd.distance = v.length();
		v /= ypd.distance;

		// Yaw
		spn::Vec3 xzvec(v.x, 0, v.z);
		if(xzvec.len_sq() < 1e-6f)
			ypd.yaw.set(0);
		else {
			xzvec.normalize();
			float ac = std::acos(Saturate(xzvec.z, 1.f));
			if(xzvec.x < 0)
				ac = 2*spn::Pi<float> - ac;
			ypd.yaw.set(ac);
		}

		// Pitch
		constexpr float h = AngleInfo<Radian_t>::oneloop<float> / 2.f;
		ypd.pitch.set(Saturate(std::asin(v.y), -h, h));
		return ypd;
	}
	std::pair<spn::Vec3, spn::Quat> YawPitchDist::toOffsetRot() const {
		// Z軸をYaw/Pitch/Roll傾けた方向に対してDist距離進んだ場所がカメラの位置
		// カメラの方向は変換済みZ軸と逆
		spn::AQuat q = spn::AQuat::RotationYPR(yaw, pitch, RadF(0));
		spn::AVec3 z = q.getDir();
		spn::Vec3 pos(z*distance);
		spn::Vec3 vd = -(z*distance).normalization();
		spn::Quat rot(spn::Quat::LookAt(vd, spn::Vec3(0,1,0)));
		return std::make_pair(pos, rot);
	}
	std::ostream& operator << (std::ostream& os, const YawPitchDist& ypd) {
		os << boost::format("YPD: yaw=%1% pitch=%2% dist=%3%") % ypd.yaw % ypd.pitch % ypd.distance;
		return os;
	}

	// ------------ AffineParts ------------
	AffineParts AffineParts::Decomp(const AMat43& m) {
		AMat33 tm;
		AffineParts ap;
		// オフセットは4行目をそのまま抽出
		ap.offset = m.getRow(3);
		for(int i=0 ; i<3 ; i++) {
			auto r = m.getRow(i);
			ap.scale.m[i] = r.length();
			tm.getRow(i) = r * Rcp22Bit(ap.scale.m[i]);
		}
		// 掌性チェックして、左手系でなければX軸を反転
		auto cv = tm.getRow(1).cross(tm.getRow(2));
		if(tm.getRow(0).dot(cv) <= 0) {
			tm.getRow(0) *= -1;
			ap.scale.x *= -1;
		}

		ap.rotation = Quat::FromAxis(tm.getRow(0), tm.getRow(1), tm.getRow(2));
		return ap;
	}
	std::string AddLineNumber(const std::string& src, int numOffset, int viewNum, bool bPrevLR, bool bPostLR) {
		std::string::size_type pos[2] = {};
		std::stringstream ss;
		if(bPrevLR)
			ss << std::endl;
		int lnum = numOffset;
		for(;;) {
			if(lnum >= viewNum)
				ss << (boost::format("%5d:\t") % lnum);
			else
				ss << "      \t";
			++lnum;
			pos[1] = src.find_first_of('\n', pos[0]);
			if(pos[1] == std::string::npos) {
				ss.write(&src[pos[0]], src.length()-pos[0]);
				break;
			}
			ss.write(&src[pos[0]], pos[1]-pos[0]);
			ss << std::endl;
			pos[0] = pos[1]+1;
		}
		if(bPostLR)
			ss << std::endl;
		return ss.str();
	}
	namespace {
		// 行分離用Regex
		const static boost::regex re_line("^(.+?)(\\n|$)");
		template <class CB>
		void RegexFindCallback(const std::string& str, const boost::regex& re, CB cb) {
			boost::smatch m, m2;
			int lnum = 0;
			auto itrB = str.cbegin(),
				itrE = str.cend();
			// まず行で分ける
			while(boost::regex_search(itrB, itrE, m, re_line)) {
				// ユーザー指定のRegexでチェック
				if(boost::regex_search(m[1].first, m[1].second, m2, re)) {
					if(!cb(lnum, m))
						break;
				}
				itrB = m[0].second;
				++lnum;
			}
		}
	}
	OPLinePos RegexFindFirst(const std::string& str, const boost::regex& re) {
		OPLinePos ret;
		RegexFindCallback(str, re, [&ret](int lnum, const boost::smatch& m)->bool{
			ret = std::make_pair(lnum, m[0].first);
			return false;
		});
		return ret;
	}
	OPLinePos RegexFindLast(const std::string& str, const boost::regex& re) {
		OPLinePos ret;
		RegexFindCallback(str, re, [&ret](int lnum, const boost::smatch& m)->bool{
			ret = std::make_pair(lnum, m[0].first);
			return true;
		});
		return ret;
	}

	bool IsInTriangle(const Vec3& vtx0, const Vec3& vtx1, const Vec3& vtx2, const Vec3& pos) {
		Vec3 normal = *NormalFromPoints(vtx0, vtx1, vtx2);

		// 1st Triangle
		Vec3 tv = Vec3(vtx1-vtx0) % (pos-vtx0);
		float lensq = tv.len_sq();
		if(lensq > 1e-5f) {
			tv /= lensq;
			if(normal.dot(tv) < -1e-4f)
				return false;
		}

		// 2nd Triangle
		tv = Vec3(vtx2-vtx1) % (pos-vtx1);
		lensq = tv.len_sq();
		if(lensq > 1e-5f) {
			tv /= lensq;
			if(normal.dot(tv) < -1e-4f)
				return false;
		}

		// 3rd Triangle
		tv = Vec3(vtx0-vtx2) % (pos-vtx2);
		lensq = tv.len_sq();
		if(lensq > 1e-5f) {
			tv /= lensq;
			if(normal.dot(tv) < -1e-4f)
				return false;
		}
		return true;
	}
}
