#pragma once
#include <fstream>
#include <memory>

namespace spn {
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
		using UP_Ist = std::unique_ptr<std::istream>;
		const static std::ios_base::seekdir cs_flag[];
		UP_Ist			_up;
		std::istream*	_is;

		//! 参照だけ借りて初期化（開放しない）
		AdaptStd(std::istream& is);
		//! デストラクタで開放する
		AdaptStd(UP_Ist&& u);
		AdaptStd(const AdaptStd&) = default;
		AdaptStd& read(void* dst, streamsize len) override;
		AdaptStd& seekg(streamoff dist, Dir dir) override;
		streampos tellg() const override;
	};
	struct AdaptOStd : AdaptOStream {
		using UP_Ost = std::unique_ptr<std::ostream>;
		UP_Ost			_up;
		std::ostream*	_os;

		AdaptOStd(std::ostream& os);
		AdaptOStd(UP_Ost&& u);
		AdaptOStd(const AdaptOStd&) = default;
		AdaptOStd& write(const void* src, streamsize len) override;
		AdaptOStd& seekp(streamoff dist, Dir dir) override;
		streampos tellp() const override;
	};
	struct AdaptIOStd : AdaptStd, AdaptOStd {
		AdaptIOStd(std::iostream& ios);
	};
	using UP_Adapt = std::unique_ptr<AdaptStream>;
}
