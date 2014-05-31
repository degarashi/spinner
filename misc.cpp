#include "misc.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#include <boost/regex.hpp>
#pragma GCC diagnostic pop

namespace spn {
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
	AffineParts DecompAffine(const AMat43& m) {
		AMat33 tm;
		AffineParts ap;
		// オフセットは4行目をそのまま抽出
		ap.offset = m.getRow(3);
		AVec3 colm[3];
		for(int i=0 ; i<3 ; i++) {
			auto r = m.getRow(i);
			ap.scale.m[i] = r.length();
			tm.getRow(i) = r * Rcp22Bit(ap.scale.m[i]);
		}
		for(int i=0 ; i<3 ; i++)
			colm[i] = tm.getColumn(i);

		ap.rotation = Quat::FromAxis(colm[0], colm[1], colm[2]);
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
