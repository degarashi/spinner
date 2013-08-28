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

		void _invalidate() {
			_pSrc = nullptr;
			_type = Type::Invalid;
			_size = 0;
		}

	public:
		// 中身がMovableな時にちょっと微妙だけど後で考える
		AbstBuffer(const AbstBuffer& ab) {
			_type = ab._type;
			_pSrc = ab._pSrc;
			_size = ab._size;
		}
		//! initialize by const-pointer
		AbstBuffer(const void* src, size_t sz): _type(Type::ConstPtr), _size(sz), _pSrc(reinterpret_cast<const T*>(src)) {}
		//! initialize by movable-vector
		AbstBuffer(Buff&& buff): _type(Type::Movable), _buffM(&buff), _size(buff.size()) {}
		//! initialize by const-vector
		AbstBuffer(const Buff& buff): _type(Type::Const), _buffC(&buff), _size(buff.size()) {}

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

		size_t getSize() const {
			return _size;
		}
		const T* getPtr() const {
			if(_type == Type::ConstPtr)
				return _pSrc;
			return &(*_buffC)[0];
		}
	};

	size_t GetLength(const char* str);
	size_t GetLength(const char16_t* str);
	size_t GetLength(const char32_t* str);
	template <class T>
	class AbstString : public AbstBuffer<T, std::basic_string<T>> {
		using base_type = AbstBuffer<T, std::basic_string<T>>;
		using Str = std::basic_string<T>;
		public:
			// 文字数カウント機能を追加
			AbstString(const T* src): base_type(src, GetLength(src)) {}
			AbstString(const T* src, size_t len): base_type(src, len) {}
			AbstString(Str&& str): base_type(std::move(str)) {}
			AbstString(const Str& str): base_type(str) {}
	};
	using c8Str = AbstString<char>;
	using c16Str = AbstString<char16_t>;
	using c32Str = AbstString<char32_t>;
	using c8Buff = AbstBuffer<char>;
	using c16Buff = AbstBuffer<char16_t>;
	using c32Buff = AbstBuffer<char32_t>;

	using ByteBuff = std::vector<uint8_t>;
	using U16Buff = std::vector<uint16_t>;
	using FloatBuff = std::vector<float>;
	using AB_Byte = AbstBuffer<uint8_t>;
	using AB_U16 = AbstBuffer<uint16_t>;
	using AB_Float = AbstBuffer<float>;
}
