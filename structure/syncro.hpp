#pragma once
#ifdef UNIX
	#include <atomic>
	#include <thread>
#endif

namespace spn {
	#ifdef UNIX
	//! スピンロックによるmutex
	class Synchro {
		private:
			using AThID = std::atomic<std::thread::id>;

			std::atomic_bool	_abLock;	//!< 誰かロックしてるか
			AThID				_idTh;		//!< ロック中のスレッド番号
			int					_iLockNum;	//!< 再帰的なロック用カウンタ

			template <bool Block>
			bool _lock() {
				auto thid = std::this_thread::get_id();
				do {
					bool b = false;
					if(_abLock.compare_exchange_weak(b, true, std::memory_order_acquire)) {
						_iLockNum = 1;
						_idTh.store(thid, std::memory_order_relaxed);
						return true;
					}
					if(_idTh.compare_exchange_weak(thid, thid, std::memory_order_acquire)) {
						++_iLockNum;
						return true;
					}
				} while(Block);
				return false;
			}
		public:
			Synchro(): _abLock(false) {}

			void lock() { _lock<true>(); }
			bool try_lock() { return _lock<false>(); }
			void unlock() {
				if(--_iLockNum == 0)
					_abLock.store(false, std::memory_order_relaxed);
			}
	};
	#endif
	//! 何もしないダミーmutex
	class PseudoSynch {
		public:
			void lock() {}
			bool try_lock() { return true; }
			void unlock() {}
	};
}

