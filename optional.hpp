#pragma once
#include <cstdint>
#include <algorithm>
#include "error.hpp"
#include "bits.hpp"
#include <boost/serialization/access.hpp>
#include "serialization/chars.hpp"
#include "serialization/traits.hpp"

namespace spn {
	extern none_t none;
	namespace {
		namespace optional_tmp {
			struct alignas(16) Align128 {};
			struct alignas(32) Align256 {};
			struct alignas(64) Align512 {};
			using CTAlign = CType<uint8_t, uint16_t, uint32_t, uint64_t, Align128, Align256, Align512>;
			template <int NByte, int AByte>
			struct AlignedBuff {
				using AlignType = typename CTAlign::template At<CTBit::MSB_N<AByte>::result>::type;
				union {
					uint8_t 	_buffer[NByte];
					AlignType	_align_dummy;
				};
			};
		}
	}
	template <class T>
	struct _OptionalBuff : optional_tmp::AlignedBuff<sizeof(T), std::alignment_of<T>::value>,
		boost::serialization::traits<_OptionalBuff<T>, boost::serialization::object_serializable, boost::serialization::track_never, 0>
	{
		using base = optional_tmp::AlignedBuff<sizeof(T), std::alignment_of<T>::value>;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & castT();
		}

		_OptionalBuff() = default;
		template <class T2>
		_OptionalBuff(T2&& t) {
			operator=(std::forward<T2>(t));
		}
		template <class... Ts>
		void operator = (ArgHolder<Ts...>&& ah) {
			void* ptr = base::_buffer;
			auto fn = [ptr](Ts... ts) {
				new(ptr) T(ts...);
			};
			ah.inorder(fn);
		}
		template <class T2>
		void operator = (T2&& t) {
			new(base::_buffer) T(std::forward<T2>(t));
		}
		T& castT() { return *reinterpret_cast<T*>(base::_buffer); }
		const T& castCT() const { return *reinterpret_cast<const T*>(base::_buffer); }
		void dtor() { castT().~T(); }
		template <class T2>
		void default_ctor() { new(base::_buffer) T2(); }
	};

	template <class T>
	struct _OptionalBuff<T&> : boost::serialization::traits<_OptionalBuff<T&>, boost::serialization::object_serializable, boost::serialization::track_selectively, 0> {
		T*		_buffer;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & _buffer;
		}

		_OptionalBuff() = default;
		_OptionalBuff(T* t): _buffer(t) {}
		template <class T2>
		_OptionalBuff(T2&& t): _buffer(&t) {}
		void operator = (T&& t) { _buffer = &t; }
		void operator = (T& t) { _buffer = &t; }
		T& castT() { return *_buffer; }
		const T& castCT() const { return *_buffer; }
		void dtor() {}
		template <class T2>
		void default_ctor() {}
	};

	template <class T>
	struct _OptionalBuff<T*> : boost::serialization::traits<_OptionalBuff<T*>, boost::serialization::object_serializable, boost::serialization::track_selectively, 0> {
		T*		_buffer;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & _buffer;
		}

		_OptionalBuff() = default;
		_OptionalBuff(T* t): _buffer(t) {}
		void operator = (T* t) { _buffer = t; }
		T*& castT() { return _buffer; }
		const T*& castCT() const { return _buffer; }
		void dtor() {}
		template <class T2>
		void default_ctor() {}
	};
	template <class... Ts>
	auto construct(Ts&&... ts) -> ArgHolder<Ts...> {
		return ArgHolder<Ts...>(std::forward<Ts>(ts)...);
	}

	//! noseq_listでboost::optionalを使おうとしたらよくわからないエラーが出たので自作してしまった
	template <class T>
	class Optional {
		private:
			template <class T2>
			friend class Optional;
			template <class A>
			struct IsOpt : std::false_type {};
			template <class A>
			struct IsOpt<Optional<A>> : std::true_type {};

			using Buffer = _OptionalBuff<T>;
			Buffer	_buffer;
			bool	_bInit;

			void t__release() noexcept {
				_buffer.dtor();
				_bInit = false;
			}
			void _release() noexcept {
				if(_bInit)
					t__release();
			}
			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int) {
				if(typename Archive::is_loading())
					_release();
				ar & _bInit;
				if(_bInit) {
					if(typename Archive::is_loading())
						_buffer.template default_ctor<T>();
					ar & _buffer;
				}
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
			Optional(T2&& t, typename std::enable_if<
								!IsOpt<
									typename std::decay<T2>::type
								>::value
							>::type* = nullptr):
				_buffer(std::forward<T2>(t)), _bInit(true) {}
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
			// 暗黙の型変換は不正
			// invalid implicit data convertion
			template <class TC>
			operator TC () const = delete;
			template <class = void>
			operator bool () const {
				return _bInit;
			}

			typename std::add_pointer<decltype(_buffer.castT())>::type operator -> () {
				return &get();
			}
			typename std::add_pointer<decltype(_buffer.castCT())>::type operator -> () const {
				return &get();
			}
			bool operator == (const Optional& t) const {
				bool b = _bInit;
				if(b == t._bInit) {
					if(b)
						return get() == t.get();
					return true;
				}
				return false;
			}
			bool operator != (const Optional& t) const {
				return !(this->operator == (t));
			}
			Optional& operator = (const Optional<T>& t) {
				_release();
				if(t) {
					_buffer = t._buffer.castCT();
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
			template <class T2,
						class = T2,
						class = typename std::enable_if<
								!IsOpt<
									typename std::decay<T2>::type
								>::value
							>::type
						>
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
			Optional& operator = (_AsInitialized) noexcept {
				_release();
				_bInit = true;
				return *this;
			}
	};
	template <class T>
	const typename Optional<T>::_AsInitialized Optional<T>::AsInitialized{};

	namespace optional_tmp {}
}
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class), spn::Optional, object_serializable)

