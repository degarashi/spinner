#pragma once
#include <vector>
#include <cstdint>
#include <cassert>
#include <string>

namespace spn {
	//! 複数のバッファ形式を1つに纏める
	template <class T, class Buff=std::vector<T>>
	class AbstBuffer {
		enum class Type {
			ConstPtr,
			Movable,
			Const,
			Invalid
		};
		Type _type;
		union {
			const T*	_pSrc;
			Buff*		_buffM;
			const Buff*	_buffC;
		};
		size_t		_size;

		//! 内部メモリを解放せずに無効化
		void _invalidate() noexcept {
			_pSrc = nullptr;
			_type = Type::Invalid;
			_size = 0;
		}

	public:
		AbstBuffer(const AbstBuffer& ab) noexcept {
			_pSrc = ab._pSrc;
			_size = ab._size;
			if((_type = ab._type) == Type::Movable) {
				// データをコピー
				_buffM = new Buff(*ab._buffM);
			}
		}
		AbstBuffer(AbstBuffer&& ab) noexcept {
			_type = ab._type;
			_pSrc = ab._pSrc;
			_size = ab._size;
			if(_type == Type::Movable)
				ab._invalidate();
		}
		~AbstBuffer() {
			if(_type == Type::Movable)
				delete _buffM;
		}
		AbstBuffer() noexcept: AbstBuffer(nullptr) {}
		AbstBuffer(std::nullptr_t) noexcept: AbstBuffer(nullptr, 0) {}
		//! initialize by const-pointer
		AbstBuffer(const void* src, size_t sz) noexcept : _type(Type::ConstPtr), _pSrc(reinterpret_cast<const T*>(src)), _size(sz) {}
		//! initialize by movable-vector
		AbstBuffer(Buff&& buff): _type(Type::Movable), _buffM(new Buff(std::move(buff))), _size(_buffM->size()) {}
		//! initialize by const-vector
		AbstBuffer(const Buff& buff) noexcept: _type(Type::Const), _buffC(&buff), _size(buff.size()) {}

		AbstBuffer& operator = (AbstBuffer&& a) noexcept {
			_type = a._type;
			_buffM = a._buffM;
			_size = a._size;
			if(_type == Type::Movable)
				a._invalidate();
			return *this; }
		AbstBuffer& operator = (const AbstBuffer& a) noexcept {
			_type = a._type;
			_buffM = a._buffM;
			_size = a._size;
			if(_type == Type::Movable) {
				// データをコピー
				_buffM = new Buff(*a._buffM);
			}
			return *this;
		}
		//! バッファを外部に出力 (movableはmove)
		void setTo(Buff& dst) {
			switch(_type) {
				case Type::ConstPtr:
					dst.assign(_pSrc, _pSrc+_size);
					break;
				case Type::Movable:
					dst = std::move(*_buffM);
					delete _buffM;
					_invalidate();
					break;
				case Type::Const:
					dst = *_buffC;
					break;
				default:
					assert(false);
					break;
			}
		}
		Buff moveTo() {
			switch(_type) {
				case Type::ConstPtr:
					return Buff(_pSrc, _pSrc+_size);
				case Type::Movable: {
					Buff ret(std::move(*_buffM));
					delete _buffM;
					_invalidate();
					return ret; }
				case Type::Const:
						return *_buffC;
				default:
					assert(false);
			}
			return Buff();
		}

		//! データのサイズ (not 文字列長)
		size_t getLength() const noexcept {
			return _size;
		}
		//! データの先頭ポインタを取得 (null-terminatedは保証されない)
		const T* getPtr() const noexcept {
			if(_type == Type::ConstPtr)
				return _pSrc;
			return &(*_buffC)[0];
		}
		bool empty() const noexcept {
			return getLength() == 0;
		}
	};

	struct StrLen {
		size_t	dataLen,	//!< 単純なバイト長
				strLen;		//!< 多バイト文字を考慮した文字列長
		StrLen() = default;
		StrLen(size_t sz): dataLen(sz), strLen(sz) {}
		StrLen(size_t dLen, size_t sLen): dataLen(dLen), strLen(sLen) {}
	};
	// ---- 文字列バイトの長さが不明な場合に文字数と一緒に計算 ----
	StrLen GetLength(const char* str);
	StrLen GetLength(const char16_t* str);
	StrLen GetLength(const char32_t* str);
	// ---- 文字列バイトの長さが分かっている場合の処理 ----
	StrLen GetLength(const char* str, size_t len);
	StrLen GetLength(const char16_t* str, size_t len);
	StrLen GetLength(const char32_t* str, size_t len);
	// とりあえずバイト長だけ格納しておいて必要に応じて文字列長を計算
	template <class T>
	class AbstString : public AbstBuffer<T, std::basic_string<T>> {
		using base_type = AbstBuffer<T, std::basic_string<T>>;
		using Str = std::basic_string<T>;
		mutable StrLen	_strLenP;
		mutable bool	_bStrLen;	//!< 文字列長を持っている場合はtrue
		mutable bool	_bNonNull;	//!< non null-terminatedでない場合にtrue (バイト長は必ず持っている条件)
		public:
			AbstString() noexcept: AbstString(nullptr) {}
			AbstString(std::nullptr_t) noexcept:
				base_type(nullptr),
				_bStrLen(false),
				_bNonNull(true)
			{}
			// 文字数カウント機能を追加
			AbstString(const T* src):
				base_type(src, (_strLenP=GetLength(src)).dataLen),
				_bStrLen(true),
				_bNonNull(false)
			{}
			// Not NullTerminatedかもしれないのでフラグを立てておく
			AbstString(const T* src, size_t dataLen):
				base_type(src, dataLen),
				_bStrLen(false),
				_bNonNull(true)
			{}
			AbstString(const Str& str):
				base_type(str),
				_bStrLen(false),
				_bNonNull(false)
			{}
			AbstString(Str&& str):
				base_type(std::move(str)),
				_bStrLen(false),
				_bNonNull(false)
			{}
			AbstString(const AbstString& str) noexcept:
				base_type(str),
				_strLenP(str._strLenP),
				_bStrLen(str._bStrLen),
				_bNonNull(str._bNonNull)
			{}
			AbstString(AbstString&& str) noexcept:
				base_type(std::move(str)),
				_strLenP(str._strLenP),
				_bStrLen(str._bStrLen),
				_bNonNull(str._bNonNull)
			{}

			AbstString& operator = (AbstString&& a) noexcept {
				this->~AbstString();
				new(this) AbstString(std::move(a));
				return *this;
			}
			AbstString& operator = (const AbstString& a) noexcept {
				reinterpret_cast<const base_type&>(*this) = reinterpret_cast<const base_type&>(a);
				_strLenP = a._strLenP;
				_bStrLen = a._bStrLen;
				_bNonNull = a._bNonNull;
				return *this;
			}

			size_t getStringLength() const {
				if(!_bStrLen) {
					_bStrLen = true;
					auto len = GetLength(base_type::getPtr(), base_type::getLength());
					assert(len.dataLen == base_type::getLength());
					_strLenP.strLen = len.strLen;
				}
				return _strLenP.strLen;
			}
			//! 文字列の先頭ポインタを取得 (null-terminated保証付き)
			const T* getStringPtr() const {
				// フラグが立ってない場合はgetPtr()と同義
				if(_bNonNull) {
					// 1+の領域分コピーして最後をnullにセット
					auto len = base_type::getLength();
					AbstString tmp(base_type::getPtr(), len+1);
					const_cast<T*>(tmp.getPtr())[len] = T(0);
					auto* self = const_cast<AbstString*>(this);
					std::swap(*self, tmp);
				}
				return base_type::getPtr();
			}
	};
	using c8Str = AbstString<char>;
	using c16Str = AbstString<char16_t>;
	using c32Str = AbstString<char32_t>;

	using ByteBuff = std::vector<uint8_t>;
	using U16Buff = std::vector<uint16_t>;
	using FloatBuff = std::vector<float>;
	using AB_Byte = AbstBuffer<uint8_t>;
	using AB_U16 = AbstBuffer<uint16_t>;
	using AB_Float = AbstBuffer<float>;

	struct ToNStr {
		template <class T>
		static AbstString<T> MakeABS(const T* src);
		template <class T>
		static AbstString<T> MakeABS(const T* src, size_t dataLen);
		template <class T>
		static AbstString<T> MakeABS(const std::basic_string<T>& s);
		template <class T>
		static AbstString<T> MakeABS(std::basic_string<T>&& s);
		template <class T>
		static AbstString<T> MakeABS(const AbstString<T>& s);
		template <class T>
		static AbstString<T> MakeABS(AbstString<T>&& s);
	};
	namespace Text {
		std::u16string UTFConvertTo16(c8Str src);
		std::u16string UTFConvertTo16(c32Str src);
		std::u32string UTFConvertTo32(c16Str src);
		std::u32string UTFConvertTo32(c8Str src);
		std::string UTFConvertTo8(c16Str src);
		std::string UTFConvertTo8(c32Str src);

		struct Code {
			char32_t	code;
			int			nread, nwrite;
		};
		Code UTF16To32(char32_t src);
		Code UTF32To16(char32_t src);
		Code UTF8To32(char32_t src);
		Code UTF32To8(char32_t src);
		Code UTF8To16(char32_t src);
		Code UTF16To8(char32_t src);
	}
	template <class TO, class FROM> inline AbstString<TO> UTFConvertToN(FROM);
	template <class TO> inline AbstString<TO> UTFConvertToN(AbstString<TO> src) { return src; }
	template <> inline c16Str UTFConvertToN<char16_t>(c8Str src) { return Text::UTFConvertTo16(src); }
	template <> inline c32Str UTFConvertToN<char32_t>(c8Str src) { return Text::UTFConvertTo32(src); }
	template <> inline c8Str UTFConvertToN<char>(c16Str src) { return Text::UTFConvertTo8(src); }
	template <> inline c32Str UTFConvertToN<char32_t>(c16Str src) { return Text::UTFConvertTo32(src); }
	template <> inline c8Str UTFConvertToN<char>(c32Str src) { return Text::UTFConvertTo8(src); }
	template <> inline c16Str UTFConvertToN<char16_t>(c32Str src) { return Text::UTFConvertTo16(src); }

	template <class TO, class FROM> inline Text::Code UTFToN(FROM);
	template <class TO> inline Text::Code UTFToN(const TO* c) {
		return Text::Code{static_cast<char32_t>(*c),
							sizeof(TO), sizeof(TO)};
	}
	template <> inline Text::Code UTFToN<char16_t>(const char* c) { return Text::UTF8To16(*reinterpret_cast<const char32_t*>(c)); }
	template <> inline Text::Code UTFToN<char32_t>(const char* c) { return Text::UTF8To32(*reinterpret_cast<const char32_t*>(c)); }
	template <> inline Text::Code UTFToN<char>(const char16_t* c) { return Text::UTF16To8(*reinterpret_cast<const char32_t*>(c)); }
	template <> inline Text::Code UTFToN<char32_t>(const char16_t* c) { return Text::UTF16To32(*reinterpret_cast<const char32_t*>(c)); }
	template <> inline Text::Code UTFToN<char>(const char32_t* c) { return Text::UTF32To8(*reinterpret_cast<const char32_t*>(c)); }
	template <> inline Text::Code UTFToN<char16_t>(const char32_t* c) { return Text::UTF32To16(*reinterpret_cast<const char32_t*>(c)); }
	template <class T>
	class ToNStrT : public AbstString<T> {
		using base = AbstString<T>;
		public:
			ToNStrT() = default;
			template <class T2>
			ToNStrT(const AbstString<T2>& c): base(UTFConvertToN<T>(c)) {}
			template <class T2>
			ToNStrT(AbstString<T2>&& c): base(UTFConvertToN<T>(c)) {}

			ToNStrT& operator = (ToNStrT&& c) noexcept= default;
			ToNStrT& operator = (const ToNStrT& c) noexcept = default;
			template <class... Ts>
			ToNStrT(Ts&&... ts): ToNStrT(decltype(ToNStr::MakeABS(std::forward<Ts>(ts)...))(std::forward<Ts>(ts)...)) {}
	};
	using To8Str = ToNStrT<char>;
	using To16Str = ToNStrT<char16_t>;
	using To32Str = ToNStrT<char32_t>;
}
