
namespace spn {
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
		_OptionalBuff(T& t): _buffer(&t) {}
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
		T* castT() { return _buffer; }
		const T* castCT() const { return _buffer; }
		void dtor() {}
	};

	//! noseq_listでboost::optionalを使おうとしたらよくわからないエラーが出たので自作してしまった
	template <class T>
	class Optional {
		private:
			_OptionalBuff<T> _buffer;
			bool	_bInit;

			void _release() {
				if(_bInit) {
					_buffer.dtor();
					_bInit = false;
				}
			}

		public:
			Optional(): _bInit(false) {}
			Optional(boost::none_t): _bInit(false) {}
			template <class T2>
			Optional(T2&& t): _buffer(std::forward<T2>(t)), _bInit(true) {}
			Optional(Optional<T>&& t): _bInit(t) {
				if(t) {
					_buffer = std::move(t._buffer.castT());
					t._bInit = false;
				}
			}
			Optional(const Optional<T>& t): _bInit(t) {
				if(t)
					_buffer = t._buffer.castT();
			}
			~Optional() {
				_release();
			}

			decltype(_buffer.castT()) get() {
				if(!*this)
					throw boost::bad_get();
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

			template <class T2>
			Optional& operator = (T2&& t) {
				_release();
				_buffer = std::forward<T2>(t);
				_bInit = true;
				return *this;
			}
			Optional& operator = (boost::none_t) noexcept {
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
			Optional& operator = (Optional<T>&& t) noexcept {
				_release();
				if(t) {
					_buffer = std::move(t._buffer.castT());
					t._bInit = false;
					_bInit = true;
				}
				return *this;
			}
	};
}
