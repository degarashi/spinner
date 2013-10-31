#pragma once
#include <fstream>

namespace spn {
	// compatible interface of std::stream and SDL
	struct AdaptStreamBase {
		enum Dir { beg, cur, end };
		using streampos = typename std::istream::streampos;
		using streamoff = typename std::istream::streamoff;
		using streamsize = std::streamsize;
	};
	struct AdaptStream : AdaptStreamBase {
		virtual AdaptStream& read(void* dst, streamsize len) = 0;
		virtual AdaptStream& seekg(streamoff dast, Dir dir) = 0;
		virtual streampos tellg() const = 0;
	};
	struct AdaptOStream : AdaptStreamBase {
		virtual AdaptOStream& write(const void* src, streamsize len) = 0;
		virtual AdaptOStream& seekp(streamoff dist, Dir dir) = 0;
		virtual streampos tellp() const = 0;
	};
	struct AdaptStd : AdaptStream {
		std::istream&	_is;

		const static std::ios_base::seekdir cs_flag[];
		AdaptStd(std::istream& is);
		AdaptStd& read(void* dst, streamsize len) override;
		AdaptStd& seekg(streamoff dist, Dir dir) override;
		streampos tellg() const override;
	};
	struct AdaptOStd : AdaptOStream {
		std::ostream&	_os;

		AdaptOStd(std::ostream& os);
		AdaptOStd& write(const void* src, streamsize len) override;
		AdaptOStd& seekp(streamoff dist, Dir dir) override;
		streampos tellp() const override;
	};
	struct AdaptIOStd : AdaptStd, AdaptOStd {
		AdaptIOStd(std::iostream& ios);
	};
}
