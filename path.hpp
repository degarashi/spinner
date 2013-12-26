#pragma once
#include <string>
#include <chrono>
#include <functional>
#include <deque>
#include "abstbuff.hpp"
#include "serialization/chars.hpp"
#define BOOST_PP_VARIADICS 1
#include <boost/serialization/access.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/base_object.hpp>

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
			//! 前後の余分な区切り文字を省く
			/*! \return [NeedOperation, AbsoluteFlag] */
			template <class Itr>
			static std::pair<bool,bool> _StripSC(Itr& from, Itr& to);
        private:
            friend class boost::serialization::access;
            template <class Archive>
            void serialize(Archive& ar, const unsigned int) {
                ar & _path & _segment & _bAbsolute;
            }

		public:
			PathBlock();
			PathBlock(const PathBlock&) = default;
			PathBlock(PathBlock&& p);
			PathBlock(To32Str p);

			bool operator == (const PathBlock& p) const;
			bool operator != (const PathBlock& p) const;
			PathBlock& operator = (const PathBlock&) = default;
			PathBlock& operator = (PathBlock&& p);
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
			bool empty() const;
	};
	class URI : public PathBlock {
		const static std::string SEP;
		const static std::u32string SEP32;
		std::string		_type;

        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, const unsigned int) {
            ar & _type & boost::serialization::base_object<PathBlock>(*this);
        }

		public:
			URI();
			URI(To8Str p);
			URI(const URI& u) = default;
			URI(URI&& u);

			URI& operator = (const URI&) = default;
			URI& operator = (URI&& u);

			void setPath(To8Str p);
			const std::string& getType_utf8() const;
			std::string plainUri_utf8() const;
			std::u32string plainUri_utf32() const;
			void setType(const std::string& typ);
			const PathBlock& path() const;
			PathBlock& path();
	};

	uint64_t UnixTime2WinTime(time_t t);
	time_t WinTime2UnixTime(uint64_t ft);
	uint64_t u32_u64(uint32_t valH, uint32_t valL);

	//! ファイルアクセス時間比較用
	struct FTime {
		uint64_t	tmAccess,
					tmModify,
					tmCreated;

		bool operator == (const FTime& ft) const;
		bool operator != (const FTime& ft) const;
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
		FTime		ftime;

		FStatus() = default;
		FStatus(uint32_t flag);
		bool operator == (const FStatus& fs) const;
		bool operator != (const FStatus& fs) const;
	};
}
