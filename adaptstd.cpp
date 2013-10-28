#include "adaptstream.hpp"

namespace spn {
	const std::ios_base::seekdir AdaptStd::cs_flag[] = {std::istream::beg, std::istream::cur, std::istream::end};
	AdaptStd::AdaptStd(std::istream& is): _is(is) {}
	AdaptStd& AdaptStd::read(void* dst, streamsize len) {
		_is.read(reinterpret_cast<char*>(dst), len);
		return *this;
	}
	AdaptStd& AdaptStd::seekg(streamoff dist, Dir dir) {
		_is.seekg(dist, cs_flag[dir]);
		return *this;
	}
	AdaptStream::streampos AdaptStd::tellg() const {
		return _is.tellg();
	}
}
