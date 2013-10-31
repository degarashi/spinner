#include "adaptstream.hpp"

namespace spn {
	// ------------------ AdaptStd ------------------
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
	// ------------------ AdaptOStd ------------------
	AdaptOStd::AdaptOStd(std::ostream& os): _os(os) {}
	AdaptOStd& AdaptOStd::write(const void* src, streamsize len) {
		_os.write(reinterpret_cast<const char*>(src), len);
		return *this;
	}
	AdaptOStd& AdaptOStd::seekp(streamoff dist, Dir dir) {
		_os.seekp(dist, AdaptStd::cs_flag[dir]);
		return *this;
	}
	AdaptOStream::streampos AdaptOStd::tellp() const {
		return _os.tellp();
	}
	// ------------------ AdaptIOStd ------------------
	AdaptIOStd::AdaptIOStd(std::iostream& ios): AdaptStd(ios), AdaptOStd(ios) {}
}
