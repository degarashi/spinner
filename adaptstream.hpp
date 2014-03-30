#pragma once
#include <fstream>
#include <memory>

namespace spn {
	using UP_IStream = std::unique_ptr<std::istream>;
	using UP_OStream = std::unique_ptr<std::ostream>;
	using UP_IOStream = std::unique_ptr<std::iostream>;
	// compatible interface of std::stream and SDL
	struct AdaptStreamBase {
		enum Dir { beg, cur, end };
		using streampos = typename std::istream::streampos;
		using streamoff = typename std::istream::streamoff;
		using streamsize = std::streamsize;
		virtual ~AdaptStreamBase() {}
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
		const static std::ios_base::seekdir cs_flag[];
		UP_IStream		_up;
		std::istream*	_is;

		//! 参照だけ借りて初期化（開放しない）
		AdaptStd(std::istream& is);
		//! デストラクタで開放する
		AdaptStd(UP_IStream&& u);
		AdaptStd(const AdaptStd&) = default;
		AdaptStd& read(void* dst, streamsize len) override;
		AdaptStd& seekg(streamoff dist, Dir dir) override;
		streampos tellg() const override;
	};
	struct AdaptOStd : AdaptOStream {
		UP_OStream		_up;
		std::ostream*	_os;

		AdaptOStd(std::ostream& os);
		AdaptOStd(UP_OStream&& u);
		AdaptOStd(const AdaptOStd&) = default;
		AdaptOStd& write(const void* src, streamsize len) override;
		AdaptOStd& seekp(streamoff dist, Dir dir) override;
		streampos tellp() const override;
	};
	struct AdaptIOStd : AdaptStd, AdaptOStd {
		AdaptIOStd(std::iostream& ios);
		AdaptIOStd(UP_IOStream&& u);
	};
	using UP_Adapt = std::unique_ptr<AdaptStream>;
}
