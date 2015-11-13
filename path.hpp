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
#include <boost/serialization/vector.hpp>
#include <boost/serialization/base_object.hpp>

struct stat;
namespace spn {
	namespace detail {
		template <class T>
		struct CharConst;
		template <>
		struct CharConst<char> {
			constexpr static char SC = '/',
								DOT = '.',
								EOS = '\0',
								CLN = ':';
		};
		template <>
		struct CharConst<char32_t> {
			constexpr static char32_t SC = U'/',	//!< セパレート文字
								DOT = U'.',		//!< 拡張子の前の記号
								EOS = U'\0',	//!< 終端記号
								CLN = U':';		//!< コロン :
		};
	}
	using Int_OP = spn::Optional<int>;
	class PathBlock {
		public:
			enum {
				Beg = 0,
				End = 0xffff
			};
			using CItr = std::string::const_iterator;
			using CItr_OP = spn::Optional<CItr>;
			using ChkStringPos = std::function<bool (CItr)>;
			using CBStringNum = std::function<Int_OP (Int_OP)>;
			static Int_OP ExtGetNum(const std::string& path);
			static Int_OP PathGetNum(const std::string& path);
			static void ExtSetNum(std::string& path, const CBStringNum& cb);
			static void PathSetNum(std::string& path, const CBStringNum& cb);
		private:
			static CItr_OP _StringGetPos(CItr itrB, CItr itrE, const ChkStringPos& chk);
			static CItr_OP _StringGetExtPos(CItr itrB, CItr itrE);
			static CItr_OP _StringGetNumPos(CItr itrB, CItr itrE);
			//! 文字列末尾の数値を取り出す
			/*! 数値が無ければnoneを返す */
			static Int_OP _StringGetNum(CItr itrB, CItr itrE);
			static Int_OP _StringToNum(CItr itrB, CItr itrE);
			static void _StringSetNum(std::string& str, int beg_pos, int end_pos, const CBStringNum& cb);
		protected:
			using Path = std::deque<char32_t>;
			using Segment = std::deque<int>;
			using OPChar = spn::Optional<char>;
			//! 絶対パスにおける先頭スラッシュを除いたパス
			Path		_path;
			//! 区切り文字の前からのオフセット
			Segment		_segment;
			bool		_bAbsolute;
			OPChar		_driveLetter;		//!< ドライブ文字(Windows用。Linuxでは無視)　\0=無効

			static bool _IsSC(char32_t c);
			//! パスを分解しながらセグメント長をカウントし、コールバック関数を呼ぶ
			/*! fromからtoまで1文字ずつ見ながら区切り文字を直す */
			template <class CB>
			static void _ReWriteSC(Path::iterator from, Path::iterator to, char32_t sc, CB cb);

			template <class CB>
			void _iterateSegment(const char32_t* c, int /*len*/, char32_t /*sc*/, CB cb) {
				char32_t tc[128];
				auto* pTc = tc;
				while(*c != detail::CharConst<char32_t>::EOS) {
					if(_IsSC(*c)) {
						*pTc = detail::CharConst<char32_t>::EOS;
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
				// UTF-8文字列として書き出し
				std::string path;
				if(typename Archive::is_saving()) {
					path = plain_utf8();
					ar & BOOST_SERIALIZATION_NVP(path);
				}
				if(typename Archive::is_loading()) {
					ar & BOOST_SERIALIZATION_NVP(path);
					setPath(path);
				}
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
			std::string getSegment_utf8(int beg, int end, bool bAbs=true) const;
			std::string getHeader_utf8() const;
			std::u32string plain_utf32(bool bAbs=true) const;
			std::u32string getFirst_utf32(bool bAbs=true) const;
			std::u32string getLast_utf32() const;
			std::u32string getSegment_utf32(int beg, int end, bool bAbs=true) const;
			std::u32string getHeader_utf32() const;

			OPChar getDriveLetter() const;

			int size() const;
			int segments() const;
			void setPath(To32Str p);
			bool isAbsolute() const;
			bool hasExtention() const;
			void setExtension(To32Str ext);
			std::string getExtension(bool bRaw=false) const;
			//! 拡張子末尾の数値を取得
			/*! filename.ext123 -> 123
				filename.ext -> 0 */
			Int_OP getExtNum() const;
			//! 拡張子末尾の数値を加算
			void setExtNum(const CBStringNum& cb);
			//! ファイル名末尾の数値を取得
			Int_OP getPathNum() const;
			//! ファイル名末尾の数値を加算
			void setPathNum(const CBStringNum& cb);
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
            ar & BOOST_SERIALIZATION_NVP(_type) & BOOST_SERIALIZATION_BASE_OBJECT_NVP(PathBlock);
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
			bool operator == (const URI& u) const;
			bool operator != (const URI& u) const;

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
namespace std {
	template <>
	struct hash<spn::PathBlock> {
		std::size_t operator()(const spn::PathBlock& pb) const {
			return std::hash<std::u32string>()(pb.plain_utf32());
		}
	};
	template <>
	struct hash<spn::URI> {
		std::size_t operator()(const spn::URI& uri) const {
			return std::hash<std::u32string>()(uri.plain_utf32());
		}
	};
}
