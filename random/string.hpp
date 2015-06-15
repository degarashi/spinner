#pragma once
#include "../path.hpp"
#include "../structure/range.hpp"

namespace spn {
	namespace random {
		// --------------- string ---------------
		//! ランダムな文字列
		template <class RD, class C>
		auto GenRString(RD& rd, size_t len, const std::basic_string<C>& src) {
			std::basic_string<C> result;
			result.resize(len);
			auto srcLen = src.size();
			for(size_t i=0 ; i<len ; i++) {
				int cn = rd.template getUniform<int>({0, srcLen-1});
				result[i] = src[cn];
			}
			return std::move(result);
		}
		extern const std::basic_string<char> c_alphabet;
		//! ランダムなアルファベット列(utf-8)
		template <class RD>
		auto GenRAlphabet(RD& rd, size_t len) {
			return GenRString(rd, len, c_alphabet);
		}
		extern const std::basic_string<char> c_segment;
		//! ランダムなパスセグメント列(utf-8)
		template <class RD>
		auto GenRPathSegment(RD& rd, size_t len) {
			return GenRString(rd, len, c_segment);
		}
		//! ランダムなパス(utf-8)
		template <class RD>
		auto GenRPath(RD& rd, const RangeI& nSeg, const RangeI& nLen, const char* abs=nullptr) {
			PathBlock pb;
			if(abs)
				pb.setPath(abs);
			int nsegment = rd.template getUniform<int>({nSeg.from, nSeg.to-1});
			for(int i=0 ; i<nsegment ; i++) {
				// ランダムsegment名
				pb.pushBack(GenRPathSegment(rd,
							rd.template getUniform<int>({nLen.from, nLen.to})));
			}
			return std::move(pb);
		}
	}
}
