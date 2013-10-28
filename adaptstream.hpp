#pragma once
#include <fstream>

namespace spn {
	// compatible interface of std::stream and SDL
	struct AdaptStream {
		enum Dir { beg, cur, end };
		using streampos = typename std::istream::streampos;
		using streamoff = typename std::istream::streamoff;
		using streamsize = std::streamsize;

		virtual AdaptStream& read(void* dst, streamsize len) = 0;
		virtual AdaptStream& seekg(streamoff dast, Dir dir) = 0;
		virtual streampos tellg() const = 0;
	};
	struct AdaptStd : AdaptStream {
		std::istream&	_is;

		const static std::ios_base::seekdir cs_flag[];
		AdaptStd(std::istream& is);
		AdaptStd& read(void* dst, streamsize len) override;
		AdaptStd& seekg(streamoff dast, Dir dir) override;
		streampos tellg() const override;
	};
}
