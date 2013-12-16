#pragma once
#include <vector>
#include <cstdint>
#include <cassert>

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
		AbstBuffer* _parent = nullptr;

		//! 内部メモリを解放せずに無効化
		void _invalidate() noexcept {
			_pSrc = nullptr;
			_type = Type::Invalid;
			_size = 0;
		}

	public:
		AbstBuffer(const AbstBuffer&) = delete;
		AbstBuffer(AbstBuffer&& ab) noexcept: AbstBuffer(ab) {}
		AbstBuffer(AbstBuffer& ab) noexcept {
			_parent = &ab;
			_type = ab._type;
			_pSrc = ab._pSrc;
			_size = ab._size;
			if(ab._type == Type::Movable)
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
			return this->operator = (a); }
		AbstBuffer& operator = (AbstBuffer& a) noexcept {
			_type = a._type;
			_buffM = a._buffM;
			_size = a._size;
			if(a._type == Type::Movable)
				a._invalidate();
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
					auto* tmp = _buffM;
					_invalidate();
					return std::move(*tmp); }
				case Type::Const:
						return *_buffC;
				default:
					assert(false);
			}
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
			AbstString(std::nullptr_t) noexcept: base_type(nullptr), _bStrLen(false), _bNonNull(true) {}
			// 文字数カウント機能を追加
			AbstString(const T* src): base_type(src, (_strLenP=GetLength(src)).dataLen), _bStrLen(true), _bNonNull(false) {}
			// Not NullTerminatedかもしれないのでフラグを立てておく
			AbstString(const T* src, size_t dataLen): base_type(src, dataLen), _bStrLen(false), _bNonNull(true) {}
			AbstString(Str&& str): base_type(std::move(str)), _bStrLen(false), _bNonNull(false) {}
			AbstString(const Str& str): base_type(str), _bStrLen(false), _bNonNull(false) {}
			AbstString(AbstString& str) noexcept: base_type(str), _strLenP(str._strLenP), _bStrLen(str._bStrLen), _bNonNull(str._bNonNull) {}
			AbstString(AbstString&& str) noexcept: AbstString(str) {}
			AbstString(const AbstString&) = delete;

			AbstString& operator = (AbstString&& a) noexcept {
				return this->operator = (a); }
			AbstString& operator = (AbstString& a) noexcept {
				reinterpret_cast<base_type&>(*this) = reinterpret_cast<base_type&>(a);
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
		static AbstString<T> MakeABS(std::basic_string<T>&& s);
		template <class T>
		static AbstString<T> MakeABS(std::basic_string<T>& s);
		template <class T>
		static AbstString<T> MakeABS(const std::basic_string<T>& s);
		template <class T>
		static AbstString<T> MakeABS(AbstString<T>& s);
		template <class T>
		static AbstString<T> MakeABS(AbstString<T>&& s);
	};
	class To32Str : public c32Str {
		public:
			To32Str() = default;
			To32Str(c8Str c);
			To32Str(c16Str c);
			To32Str(c32Str& c) noexcept;
			To32Str(c32Str&& c) noexcept;
			template <class... Ts>
			To32Str(Ts&&... ts): To32Str(decltype(ToNStr::MakeABS(std::forward<Ts>(ts)...))(std::forward<Ts>(ts)...)) {}
	};
	class To16Str : public c16Str {
		public:
			To16Str() = default;
			To16Str(c32Str c);
			To16Str(c8Str c);
			To16Str(c16Str& c) noexcept;
			To16Str(c16Str&& c) noexcept;
			template <class... Ts>
			To16Str(Ts&&... ts): To16Str(decltype(ToNStr::MakeABS(std::forward<Ts>(ts)...))(std::forward<Ts>(ts)...)) {}
	};
	class To8Str : public c8Str {
		public:
			To8Str() = default;
			To8Str(c32Str c);
			To8Str(c16Str c);
			To8Str(c8Str& c) noexcept;
			To8Str(c8Str&& c) noexcept;
			template <class... Ts>
			To8Str(Ts&&... ts): To8Str(decltype(ToNStr::MakeABS(std::forward<Ts>(ts)...))(std::forward<Ts>(ts)...)) {}
	};
}
