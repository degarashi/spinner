#pragma once
#include <string>
#include <chrono>
#include <functional>
#include <deque>
#include "abstbuff.hpp"

struct stat;
namespace spn {
	class PathBlock {
		protected:
			using Path = std::deque<char32_t>;
			using Segment = std::deque<int>;
			//! 絶対パスにおける先頭スラッシュを除いたパス
			Path		_path;
			//! 区切り文字の前からのオフセット
			Segment		_segment;
			const static char32_t SC,		//!< セパレート文字
								DOT,		//!< 拡張子の前の記号
								EOS;		//!< 終端記号
			bool		_bAbsolute;

			static bool _IsSC(char32_t c);
			//! パスを分解しながらセグメント長をカウントし、コールバック関数を呼ぶ
			/*! fromからtoまで1文字ずつ見ながら区切り文字を直す */
			template <class Itr, class CB>
			static void _ReWriteSC(Itr from, Itr to, char32_t sc, CB cb);

			static int _ExtGetNum(const std::string& ext);
			static int _ExtIncNum(std::string& ext, int n=1);
			template <class CB>
			void _iterateSegment(const char32_t* c, int len, char32_t sc, CB cb) {
				char32_t tc[128];
				auto* pTc = tc;
				while(*c != EOS) {
					if(_IsSC(*c)) {
						*pTc = EOS;
						cb(tc, pTc-tc);
						pTc = tc;
					} else
						*pTc++ = *c;
					++c;
				}
			}

		public:
			PathBlock();
			PathBlock(const PathBlock&) = default;
			PathBlock(PathBlock&& p);
			PathBlock(To32Str p);

			PathBlock& operator <<= (To32Str elem);

			void pushBack(To32Str elem);
			void popBack();
			void pushFront(To32Str elem);
			void popFront();

			std::string plain_utf8(bool bAbs=true) const;
			std::string getFirst_utf8(bool bAbs=true) const;
			std::string getLast_utf8() const;
			std::u32string plain_utf32(bool bAbs=true) const;
			std::u32string getFirst_utf32(bool bAbs=true) const;
			std::u32string getLast_utf32() const;

			int size() const;
			int segments() const;
			void setPath(To32Str p);
			bool isAbsolute() const;
			bool hasExtention() const;
			void setExtension(To32Str ext);
			std::string getExtension(bool bRaw=false) const;
			int getExtNum() const;
			int addExtNum(int n=1);
			void clear();
	};

	struct FStatus {
		enum Flag : uint32_t {
			UserRead = 0x100,
			UserWrite = 0x80,
			UserExec = 0x40,
			UserRWX = UserRead | UserWrite | UserExec,

			GroupRead = 0x20,
			GroupWrite = 0x10,
			GroupExec = 0x08,
			GroupRWX = GroupRead | GroupWrite | GroupExec,

			OtherRead = 0x04,
			OtherWrite = 0x02,
			OtherExec = 0x01,
			OtherRWX = OtherRead | OtherWrite | OtherExec,

			AllRead = UserRead | GroupRead | OtherRead,
			AllWrite = UserWrite | GroupWrite | OtherWrite,
			AllExec = UserExec | GroupExec | OtherExec,
			AllRW = AllRead | AllWrite,

			FileType = 0x200,
			DirectoryType = 0x400,
			NotAvailable = 0x800
		};

		uint32_t	flag;
		uint32_t	userId,
					groupId;
		uint64_t	size;
		time_t		tmAccess,
					tmModify,
					tmCreated;

		FStatus() = default;
		FStatus(uint32_t flag);
	};
	using EnumCBD = std::function<void (const char*)>;
}
