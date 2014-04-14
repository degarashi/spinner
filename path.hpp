#pragma once
#include <string>
#include <chrono>
#include <functional>
#include <deque>
#include "optional.hpp"
#include "abstbuff.hpp"
#include "serialization/chars.hpp"
#include <boost/serialization/access.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/base_object.hpp>

struct stat;
namespace spn {
	class PathBlock {
		public:
			enum {
				Beg = 0,
				End = 0xffff
			};
		protected:
			using Path = std::deque<char32_t>;
			using Segment = std::deque<int>;
			using OPChar = spn::Optional<char>;
			//! 絶対パスにおける先頭スラッシュを除いたパス
			Path		_path;
			//! 区切り文字の前からのオフセット
			Segment		_segment;
			const static char32_t SC,		//!< セパレート文字
								DOT,		//!< 拡張子の前の記号
								EOS,		//!< 終端記号
								CLN;		//!< コロン :
			bool		_bAbsolute;
			OPChar		_driveLetter;		//!< ドライブ文字(Windows用。Linuxでは無視)　\0=無効

			static bool _IsSC(char32_t c);
			//! パスを分解しながらセグメント長をカウントし、コールバック関数を呼ぶ
			/*! fromからtoまで1文字ずつ見ながら区切り文字を直す */
			template <class CB>
			static void _ReWriteSC(Path::iterator from, Path::iterator to, char32_t sc, CB cb);

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
			template <class Itr>
			static auto _GetDriveLetter(Itr from, Itr to) -> spn::Optional<typename std::decay<decltype(*from)>::type> {
				using CH = typename std::decay<decltype(*from)>::type;
				auto cnvToCh = [](char c) {
					char32_t c2 = static_cast<char32_t>(c);
					return UTFToN<CH>(reinterpret_cast<const char*>(&c2)).code;
				};
				Itr tmp_from = from;
				if(to - from >= 3) {
					auto c = *from;
					CH cA = cnvToCh('A'),
						cZ = cnvToCh('Z'),
						ca = cnvToCh('a'),
						cz = cnvToCh('z'),
						col = cnvToCh(':');
					if((c >= cA && c <= cZ) ||
							(c >= ca && c <= cz))
					{
						if(*(++from) == col &&
							_IsSC(UTFToN<char32_t>(&*(++from)).code))
							return *tmp_from;
					}
				}
				return spn::none;
			}

			void _outputHeader(std::u32string& dst, bool bAbs) const;
        private:
            friend class boost::serialization::access;
            template <class Archive>
            void serialize(Archive& ar, const unsigned int) {
                ar & _path & _segment & _bAbsolute;
            }

		public:
			template <class C>
			struct StripResult {
				using OpC = spn::Optional<C>;
				bool 	bAbsolute;
				OpC		driveLetter;
				int		nread;
			};
			template <class C>
			using OPStripResult = spn::Optional<StripResult<C>>;
			//! 前後の余分な区切り文字を省く
			/*! \return [NeedOperation, AbsoluteFlag] */
			template <class Itr>
			static auto StripSC(Itr from, Itr to) -> OPStripResult<typename std::decay<decltype(*from)>::type>;
			/*! Windowsの場合は何もせずfromを返す
				Linuxの場合は先頭のドライブ文字を削った後のポインタを返す */
			static const char* RemoveDriveLetter(const char* from, const char* to);

			PathBlock();
			PathBlock(const PathBlock&) = default;
			PathBlock(PathBlock&& p);
			PathBlock(To32Str p);

			bool operator == (const PathBlock& p) const;
			bool operator != (const PathBlock& p) const;
			PathBlock& operator = (const PathBlock&) = default;
			PathBlock& operator = (PathBlock&& p);
			PathBlock& operator <<= (To32Str elem);
			PathBlock& operator <<= (const PathBlock& p);

			//! パスを後ろに追加。引数が絶対パスの時は置き換える
			void pushBack(To32Str elem);
			void pushBack(const PathBlock& p);
			void popBack();
			//! パスを前に追加。thisが絶対パスの時は置き換える
			void pushFront(To32Str elem);
			void pushFront(const PathBlock& p);
			void popFront();

			std::string plain_utf8(bool bAbs=true) const;
			std::string getFirst_utf8(bool bAbs=true) const;
			std::string getLast_utf8() const;
			std::string getSegment_utf8(int beg, int end) const;
			std::string getHeader_utf8() const;
			std::u32string plain_utf32(bool bAbs=true) const;
			std::u32string getFirst_utf32(bool bAbs=true) const;
			std::u32string getLast_utf32() const;
			std::u32string getSegment_utf32(int beg, int end) const;
			std::u32string getHeader_utf32() const;

			OPChar getDriveLetter() const;

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
			URI(To8Str typ, To8Str path);
			URI(To8Str typ, const PathBlock& pb);

			URI& operator = (const URI&) = default;
			URI& operator = (URI&& u);

			void setPath(To8Str p);
			const std::string& getType_utf8() const;
			std::string plainUri_utf8() const;
			std::u32string plainUri_utf32() const;
			void setType(To8Str typ);
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
