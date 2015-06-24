#pragma once
#include "../path.hpp"
#include "../structure/range.hpp"

namespace spn {
	namespace random {
		// --------------- string ---------------
		//! ランダムな文字列
		template <class RDI, class C>
		auto GenRString(const RDI& rdi, size_t len, const std::basic_string<C>& src) {
			std::basic_string<C> result;
			result.resize(len);
			auto srcLen = src.size();
			for(size_t i=0 ; i<len ; i++) {
				int cn = rdi({0, srcLen-1});
				result[i] = src[cn];
			}
			return std::move(result);
		}
		extern const std::basic_string<char> c_alphabet;
		//! ランダムなアルファベット列(utf-8)
		template <class RDI>
		auto GenRAlphabet(const RDI& rdi, size_t len) {
			return GenRString(rdi, len, c_alphabet);
		}
		extern const std::basic_string<char> c_segment;
		//! ランダムなパスセグメント列(utf-8)
		template <class RDI>
		auto GenRPathSegment(const RDI& rdi, size_t len) {
			return GenRString(rdi, len, c_segment);
		}
		//! ランダムなパス(utf-8)
		template <class RDI>
		auto GenRPath(const RDI& rdi, const RangeI& nSeg, const RangeI& nLen, const char* abs=nullptr) {
			PathBlock pb;
			if(abs)
				pb.setPath(abs);
			int nsegment = rdi({nSeg.from, nSeg.to-1});
			for(int i=0 ; i<nsegment ; i++) {
				// ランダムsegment名
				pb.pushBack(GenRPathSegment(rdi,
							rdi({nLen.from, nLen.to})));
			}
			return std::move(pb);
		}
	}
}
