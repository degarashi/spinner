#pragma once
#include <cstdint>
#include <algorithm>
#include "error.hpp"

namespace spn {
	extern none_t none;
	template <class T>
	struct _OptionalBuff {
		uint8_t _buffer[sizeof(T)];

		_OptionalBuff() = default;
		template <class T2>
		_OptionalBuff(T2&& t) {
			operator=(std::forward<T2>(t));
		}
		template <class T2>
		void operator = (T2&& t) {
			new(_buffer) T(std::forward<T2>(t));
		}
		T& castT() { return *reinterpret_cast<T*>(_buffer); }
		const T& castCT() const { return *reinterpret_cast<const T*>(_buffer); }
		void dtor() { castT().~T(); }
	};

	template <class T>
	struct _OptionalBuff<T&> {
		T*		_buffer;

		_OptionalBuff() = default;
		_OptionalBuff(T* t): _buffer(t) {}
		template <class T2>
		_OptionalBuff(T2&& t): _buffer(&t) {}
		void operator = (T&& t) { _buffer = &t; }
		void operator = (T& t) { _buffer = &t; }
		T& castT() { return *_buffer; }
		const T& castCT() const { return *_buffer; }
		void dtor() {}
	};
	template <class T>
	struct _OptionalBuff<T*> {
		T*		_buffer;

		_OptionalBuff() = default;
		_OptionalBuff(T* t): _buffer(t) {}
		void operator = (T* t) { _buffer = t; }
		T*& castT() { return _buffer; }
		const T*& castCT() const { return _buffer; }
		void dtor() {}
	};
	//! noseq_listでboost::optionalを使おうとしたらよくわからないエラーが出たので自作してしまった
	template <class T>
	class Optional {
		private:
			_OptionalBuff<T> _buffer;
			bool	_bInit;

			void t__release() noexcept {
				_buffer.dtor();
				_bInit = false;
			}
			void _release() noexcept {
				if(_bInit)
					t__release();
			}

		public:
			const static struct _AsInitialized {} AsInitialized;
			//! コンストラクタは呼ばないけど初期化された物として扱う
			/*! ユーザーが自分でコンストラクタを呼ばないとエラーになる */
			Optional(_AsInitialized): _bInit(true) {}

			Optional(): _bInit(false) {}
			Optional(none_t): _bInit(false) {}
			Optional(const Optional<T>& t): _bInit(t) {
				if(t)
					_buffer = t._buffer.castCT();
			}
			Optional(Optional<T>&& t): _bInit(t) {
				if(t) {
					_buffer = std::move(t._buffer.castT());
					t.t__release();
				}
			}
			template <class T2>
			Optional(T2&& t, typename std::enable_if<!std::is_same<typename std::decay<T2>::type, Optional<T>>::value>::type* = nullptr): _buffer(std::forward<T2>(t)), _bInit(true) {}
			~Optional() {
				_release();
			}

			decltype(_buffer.castT()) get() {
				Assert(Trap, *this, "optional: bad_get")
				return _buffer.castT();
			}
			decltype(_buffer.castCT()) get() const {
				return _buffer.castCT();
			}
			decltype(_buffer.castT()) operator * () {
				return get();
			}
			decltype(_buffer.castCT()) operator * () const {
				return get();
			}
			operator bool () const {
				return _bInit;
			}

			typename std::add_pointer<decltype(_buffer.castT())>::type operator -> () {
				return &_buffer.castT();
			}
			typename std::add_pointer<decltype(_buffer.castCT())>::type operator -> () const {
				return &_buffer.castCT();
			}
			template <class T2>
			Optional& operator = (T2&& t) {
				_release();
				_buffer = std::forward<T2>(t);
				_bInit = true;
				return *this;
			}
			Optional& operator = (none_t) noexcept {
				_release();
				return *this;
			}
			Optional& operator = (const Optional<T>& t) {
				_release();
				if(t) {
					_buffer = t._buffer.castT();
					_bInit = true;
				}
				return *this;
			}
			Optional& operator = (Optional<T>&& t) {
				_release();
				if(t) {
					_buffer = std::move(t._buffer.castT());
					t.t__release();
					_bInit = true;
				}
				return *this;
			}
			Optional& operator = (_AsInitialized) noexcept {
				_release();
				_bInit = true;
				return *this;
			}
	};
	template <class T>
	const typename Optional<T>::_AsInitialized Optional<T>::AsInitialized{};
}
