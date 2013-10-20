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
		size_t	_size;
		bool	_bRelease;

		void _invalidate() {
			_pSrc = nullptr;
			_type = Type::Invalid;
			_size = 0;
		}

	public:
		AbstBuffer(const AbstBuffer& ab) {
			_type = ab._type;
			_pSrc = ab._pSrc;
			_size = ab._size;
			_bRelease = false;
		}
		AbstBuffer(AbstBuffer&& ab) {
			_type = ab._type;
			_buffM = ab._buffM;
			_size = ab._size;
			if((_bRelease = ab._bRelease)) {
				ab._bRelease = false;
				ab._buffM = nullptr;
				ab._type = Type::Invalid;
			}
		}
		~AbstBuffer() {
			if(_bRelease)
				delete _buffM;
		}
		//! initialize by const-pointer
		AbstBuffer(const void* src, size_t sz): _type(Type::ConstPtr), _pSrc(reinterpret_cast<const T*>(src)), _size(sz), _bRelease(false) {}
		//! initialize by movable-vector
		AbstBuffer(Buff&& buff): _type(Type::Movable), _buffM(new Buff(std::move(buff))), _size(_buffM->size()), _bRelease(true) {}
		//! initialize by const-vector
		AbstBuffer(const Buff& buff): _type(Type::Const), _buffC(&buff), _size(buff.size()), _bRelease(false) {}

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
				case Type::ConstPtr: return Buff(_pSrc, _pSrc+_size);
				case Type::Movable: return std::move(*_buffM);
				case Type::Const: return *_buffC;
				default: assert(false);
			}
		}

		//! データのサイズ (not 文字列長)
		size_t getSize() const {
			return _size;
		}
		const T* getPtr() const {
			if(_type == Type::ConstPtr)
				return _pSrc;
			return &(*_buffC)[0];
		}
	};

	struct StrLen {
		size_t	dataLen,	//!< 単純なバイト長
				strLen;		//!< 多バイト文字を考慮した文字列長
		StrLen() = default;
		StrLen(size_t sz): dataLen(sz), strLen(sz) {}
		StrLen(size_t dLen, size_t sLen): dataLen(dLen), strLen(sLen) {}
	};
	StrLen GetLength(const char* str);
	StrLen GetLength(const char16_t* str);
	StrLen GetLength(const char32_t* str);

	// とりあえずバイト長だけ格納しておいて必要に応じて文字列長を計算
	template <class T>
	class AbstString : public AbstBuffer<T, std::basic_string<T>> {
		using base_type = AbstBuffer<T, std::basic_string<T>>;
		using Str = std::basic_string<T>;
		mutable StrLen	_strLenP;
		mutable bool	_bStrLen;	//!< 文字列長を持っている場合はtrue
		public:
			// 文字数カウント機能を追加
			AbstString(const T* src): base_type(src, (_strLenP=GetLength(src)).dataLen), _bStrLen(true) {}
			AbstString(const T* src, size_t dataLen): base_type(src, dataLen), _bStrLen(false) {}
			AbstString(Str&& str): base_type(std::move(str)), _bStrLen(false) {}
			AbstString(const Str& str): base_type(str), _bStrLen(false) {}
			AbstString(const AbstString& str): base_type(str), _bStrLen(str._bStrLen) {}
			AbstString(AbstString&& str): base_type(std::move(str)), _bStrLen(str._bStrLen) {}

			size_t getLength() const {
				if(!_bStrLen) {
					_bStrLen = true;
					auto len = GetLength(base_type::getPtr());
					assert(len.dataLen == base_type::getSize());
					_strLenP.strLen = len.strLen;
				}
				return _strLenP.strLen;
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
}
