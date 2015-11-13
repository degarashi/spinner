#pragma once
#include "misc.hpp"
#include "bits.hpp"
#include "macro.hpp"
#include "noseq.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include "serialization/traits.hpp"
#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/binary_object.hpp>
#include "adaptstream.hpp"
#include "singleton.hpp"
#include "check_smartptr.hpp"
#include "alignedalloc.hpp"

namespace spn {
	class WHandle;
	class ResMgrBase;
	//! 型を保持しない強参照ハンドル値
	/*! インデックスに14bit, リソースIDに6bit, (デバッグ用)マジックナンバーに12bitを使用 */
	class SHandle {
		public:
			#ifdef DEBUG
				// デバッグ時のみ簡易マジックナンバーを保持
				struct HandleBits : BitDef<uint32_t, BitF<0,14>, BitF<14,6>, BitF<20,12>> {
					enum { INDEX, RESID, MAGIC }; };
			#else
				struct HandleBits : BitDef<uint32_t, BitF<0,14>, BitF<14,6>> {
					enum { INDEX, RESID }; };
			#endif
			using Value = BitField<HandleBits>;
			using VWord = Value::Word;
			using WHdl = WHandle;
		private:
			Value _value;

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & boost::serialization::make_nvp("value", _value.value());
			}
		public:
			//! デフォルト値は無効なハンドルID
			SHandle();
			SHandle(const SHandle&) = default;
			explicit SHandle(VWord w);
			#ifdef DEBUG
				SHandle(int idx, int resID, int mag);
				Value::Word getMagic() const;
			#else
				SHandle(int idx, int resID);
			#endif
			VWord getResID() const;
			VWord getIndex() const;
			VWord getValue() const;
			//! ハンドル値が有効かを判定
			bool valid() const;
			explicit operator bool () const;
			bool operator == (const SHandle& sh) const;
			bool operator != (const SHandle& sh) const;
			bool operator < (const SHandle& sh) const;
			void swap(SHandle& sh) noexcept;
			/*! ResourceIDから対応マネージャを特定して解放
				\return この呼び出しでリソースが開放された場合はtrue */
			bool release();
			void increment();
			uint32_t count() const;
			WHandle weak() const;
			void setNull();

			ResMgrBase* getManager();
			const ResMgrBase* getManager() const;
			const std::string& getResourceName() const;
	};

	//! 型を保持しない弱参照ハンドル値
	/*! インデックスに16bit, リソースIDに8bit, マジックナンバーに40bitを使用
		単なるマジックナンバー付き数値なので解放の必要は無い */
	class WHandle {
		struct MyDef : BitDef<uint64_t, BitF<0,16>, BitF<16,8>, BitF<24,40>> {
			enum { INDEX, RESID, MAGIC }; };
		public:
			using Value = BitField<MyDef>;
			using VWord = Value::Word;
		private:
			Value _value;

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & boost::serialization::make_nvp("value", _value.value());
			}
		public:
			//! デフォルト値は無効なハンドルID
			WHandle();
			WHandle(const WHandle& wh) = default;
			WHandle(int idx, int resID, VWord mag);
			explicit WHandle(VWord val);

			VWord getResID() const;
			VWord getIndex() const;
			VWord getMagic() const;
			VWord getValue() const;
			void setNull();
			bool valid() const;
			void  swap(WHandle& wh) noexcept;
			explicit operator bool () const;
			bool operator == (const WHandle& wh) const;
			bool operator != (const WHandle& wh) const;
			bool operator < (const WHandle& wh) const;
	};
}
namespace std {
	template <>
	struct hash<spn::SHandle> {
		size_t operator()(spn::SHandle sh) const {
			using Value = decltype(sh.getValue());
			return hash<Value>()(sh.getValue()); }
	};
	template <>
	struct hash<spn::WHandle> {
		size_t operator()(spn::WHandle wh) const {
			using Value = decltype(wh.getValue());
			return hash<Value>()(wh.getValue()); }
	};
}
namespace spn {
	template <class MGR, class DATA>
	class SHandleT;
	template <class MGR, class DATA>
	class WHandleT;

	namespace {
		namespace resmgr_tmp {
			template <class HDL>
			auto ReleaseHB(HDL& hdl, std::integral_constant<bool,true>) {
				return hdl.release();
			}
			template <class HDL>
			auto ReleaseHB(HDL& hdl, std::integral_constant<bool,false>) {
				SHandle sh(hdl);
				return sh.release();
			}
		}
	}
	//! 強参照スマートハンドル
	template <class HDL, bool DIRECT>
	class HdlLockB {
		protected:
			HDL _hdl;
		private:
			template <class H, bool D>
			friend class HdlLockB;
			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(_hdl);
			}
		public:
			static struct tagAsLocked {} AsLocked;
			using handle_type = HDL;
			using WHdl = typename HDL::WHdl;
			HdlLockB() = default;
			HdlLockB(HdlLockB&& hdl) { swap(hdl); }
			HdlLockB(const HdlLockB& hdl): _hdl(hdl.get()) {
				if(_hdl)
					_hdl.increment();
			}
			template <bool D>
			HdlLockB(HdlLockB<HDL,D>&& hdl): HdlLockB(reinterpret_cast<HdlLockB&&>(hdl)) {}
			template <bool D>
			HdlLockB(const HdlLockB<HDL,D>& hdl): HdlLockB(reinterpret_cast<const HdlLockB&>(hdl)) {}
			//! ハンドルを既にロック済みとして扱う
			HdlLockB(HDL hdl, tagAsLocked): _hdl(hdl) {}
			//! 参照インクリメント
			HdlLockB(HDL hdl): _hdl(hdl) {
				if(_hdl.valid())
					_hdl.increment();
			}
			HDL get() const { return _hdl; }
			HDL getBase() const { return _hdl; }
			~HdlLockB() {
				release();
			}
			bool release() {
				if(_hdl.valid()) {
					auto ret = resmgr_tmp::ReleaseHB(_hdl, std::integral_constant<bool,DIRECT>());
					_hdl.setNull();
					return ret;
				}
				return false;
			}
			//! 解放せずにハンドルを無効化する
			void setNull() {
				_hdl.setNull();
			}
			void reset(HDL hdl) {
				release();
				HdlLockB tmp(hdl);
				swap(tmp);
			}
			template <bool D>
			void swap(HdlLockB<HDL,D>& hl) noexcept { _hdl.swap(hl._hdl); }
			HdlLockB& operator = (const HdlLockB& hl) {
				reset(hl.get());
				return *this;
			}
			HdlLockB& operator = (HdlLockB&& hl) noexcept {
				swap(hl);
				return *this;
			}
			template <bool D>
			HdlLockB& operator = (const HdlLockB<HDL,D>& hl) { return operator =(reinterpret_cast<const HdlLockB&>(hl)); }
			template <bool D>
			HdlLockB& operator = (HdlLockB<HDL,D>&& hl) noexcept {
				swap(hl);
				return *this;
			}
			template <bool D>
			bool operator == (const HdlLockB<HDL,D>& hl) const {
				return _hdl == hl.get();
			}
			template <bool D>
			bool operator != (const HdlLockB<HDL,D>& hl) const {
				return !(this->operator == (hl));
			}
			HdlLockB& operator = (HDL hdl) {
				reset(hdl);
				return *this;
			}
			explicit operator bool () const {
				return valid();
			}
			operator HDL () const {
				return get();
			}

			// ---- SHandleのメソッドを仲立ち ----
			bool valid() const { return _hdl.valid(); }
			WHdl weak() const { return _hdl.weak(); }
			uint32_t count() const { return _hdl.count(); }
	};
	namespace resmgr_tmp {}
	//! 強参照スマートハンドル
	template <class HDL, bool DIRECT>
	class HdlLock : public HdlLockB<HDL,DIRECT> {
		#define DOWNCONV	typename std::enable_if<std::is_convertible<typename HDL::data_type, DATA>::value>::type
		#define UPCONV		typename std::enable_if<std::is_convertible<DATA,typename HDL::data_type>::value>::type
		#define SAMETYPE	typename std::enable_if<!std::is_same<DATA, typename HDL::data_type>::value>::type
		using base = HdlLockB<HDL, DIRECT>;
		using data_type = typename HDL::data_type;
		friend typename HDL::mgr_type;
		friend typename HDL::WHdl;
		friend HDL;
		private:
			//! 参照インクリメント(down convert)
			template <class DATA, bool D, class = DOWNCONV, class = SAMETYPE>
			HdlLock(const HdlLock<SHandleT<typename HDL::mgr_type, DATA>, D>& hl): HdlLock(hl.get()) {}
			//WARN: メモリを直接読み込んでしまっている、スマートじゃない実装
			template <class DATA, bool D, class = DOWNCONV, class = SAMETYPE>
			HdlLock(HdlLock<SHandleT<typename HDL::mgr_type, DATA>, D>&& hl): base(reinterpret_cast<base&&>(hl)) {}
		public:
			static HdlLock FromHandle(SHandle sh) { return HdlLock(sh); }

			using base::base;
			using base::operator =;
			HdlLock() = default;
			//! 参照ムーブ(カウント変更なし)
			template <class DATA, bool D, class = UPCONV>
			HdlLock(HdlLock<SHandleT<typename HDL::mgr_type, DATA>, D>&& hl) noexcept {
				//WARN: メモリを直接読み込んでしまっている、スマートじゃない実装
				base::_hdl.swap(*(HDL*)&hl);
			}
			//! 参照インクリメント(up convert)
			template <class DATA, bool D, class = UPCONV>
			HdlLock(const HdlLock<SHandleT<typename HDL::mgr_type, DATA>, D>& hl): HdlLock(hl.get()) {}

			#undef DOWNCONV
			#undef UPCONV
			#undef SAMETYPE

			// ---- SHandleのメソッドを仲立ち ----
			data_type& ref() { return base::_hdl.ref(); }
			const data_type& cref() const { return base::_hdl.cref(); }
			// ---- データにダイレクトアクセス ----
			const data_type* operator -> () const { return &base::_hdl.cref(); }
			data_type* operator -> () { return &base::_hdl.ref(); }
	};
	template <bool DIRECT>
	class HdlLock<SHandle, DIRECT> : public HdlLockB<SHandle, DIRECT> {
		using base = HdlLockB<SHandle, DIRECT>;
		public:
			using base::base;
			using base::operator =;
			HdlLock() = default;
			//TODO: コードが冗長過ぎるので要改善
			template <class T, bool D>
			HdlLock(const HdlLock<T,D>& hl): base(reinterpret_cast<const base&>(hl)) {}
			template <class T, bool D>
			HdlLock(HdlLock<T,D>&& hl): base(reinterpret_cast<base&&>(hl)) {}
			template <class T, bool D>
			HdlLock& operator = (const HdlLock<T,D>& t) { base::operator=(reinterpret_cast<const base&>(t)); return *this; }
			template <class T, bool D>
			HdlLock& operator = (HdlLock<T,D>&& t) { base::operator=(reinterpret_cast<base&&>(t)); return *this; }
	};

	//! 強参照ハンドル
	template <class MGR, class DATA = typename MGR::data_type>
	class SHandleT : public SHandle {
		friend MGR;
		friend class HdlLock<SHandleT, false>;
		friend class HdlLock<SHandleT, true>;
		private:
			// データ型をダウンコンバートする場合はMGRからしか許可しない
			template <class DAT,
						class = typename std::enable_if<std::is_convertible<DATA, DAT>::value>::type,
						class = typename std::enable_if<!std::is_same<DAT, DATA>::value>::type>
			SHandleT(const SHandleT<MGR,DAT>& sh): SHandle(sh) {}
			SHandleT(SHandle sh): SHandle(sh) {}
			void _check() {
				using mgr_data = DATA;
				using DownConv = std::is_convertible<DATA, mgr_data>;
				using UpConv = std::is_convertible<mgr_data, DATA>;
				// static_assertion: DATAからMGR::data_typeは異なっていても良いが、アップコンバートかダウンコンバート出来なければならない
				struct Check {
					const int StaticAssertion[(DownConv::value || UpConv::value) ? 0 : -1];
				};
			}

		public:
			//! 何らかの事情でSHandleからSHandleTを明示的に作成したい時に使う
			static SHandleT FromHandle(SHandle sh) { return SHandleT(sh); }
			using WHdl = WHandleT<MGR, DATA>;
			using mgr_type = MGR;
			using data_type = DATA;
			using SHandle::SHandle;
			SHandleT() = default;
			SHandleT(const SHandleT&) = default;
			// data_typeが異なっていてもアップコンバート可能ならば暗黙的な変換を許可する
			template <class DAT,
					class = typename std::enable_if<std::is_convertible<DAT, DATA>::value>::type>
			SHandleT(const SHandleT<MGR, DAT>& hdl): SHandle(hdl) {}
			// ResMgrBaseから探してメソッド呼ぶのと同じだが、こちらのほうが幾分効率が良い
			bool release() { return MGR::_ref().release(*this); }
			void increment() { MGR::_ref().increment(*this); }
			data_type& ref() {
				// Managerの保管している型とこのハンドルが返す型が異なる場合があるので明示的にキャストする
				return reinterpret_cast<data_type&>(MGR::_ref().ref(*this));
			}
			const data_type& cref() const {
				// ref()と同じ理由でキャスト
				return reinterpret_cast<const data_type&>(MGR::_ref().cref(*this));
			}
			void* getPtr() { MGR::_ref().getPtr(*this); }
			SHandle getBase() const { return *this; }

			//! 弱参照ハンドルを得る
			WHdl weak() const { return WHdl::FromHandle(MGR::_ref().weak(*this)); }
			uint32_t count() const { return MGR::_ref().count(*this); }

			explicit operator bool () const { return valid(); }
			data_type* operator -> () { return &ref(); }
			const data_type* operator -> () const { return &cref(); }
	};
	//! 弱参照ハンドル
	template <class MGR, class DATA = typename MGR::data_type>
	class WHandleT : public WHandle {
		friend MGR;
		private:
			// データ型をダウンコンバートする場合はMGRからしか許可しない
			template <class DAT,
						class = typename std::enable_if<std::is_convertible<DATA, DAT>::value>::type,
						class = typename std::enable_if<!std::is_same<DAT, DATA>::value>::type>
			WHandleT(const WHandleT<MGR,DAT>& wh): WHandle(wh) {}
			WHandleT(WHandle wh): WHandle(wh) {}
		public:
			//! 何らかの事情でSHandleからSHandleTを明示的に作成したい時に使う
			static WHandleT FromHandle(WHandle wh) { return WHandleT(wh); }
			static WHandleT FromHandle(SHandle sh) { return sh.weak(); }
			using SHdl = SHandleT<MGR, DATA>;
			using mgr_type = MGR;
			using data_type = DATA;
			using WHandle::WHandle;

			WHandleT() {
				using mgr_data = DATA;
				using DownConv = std::is_convertible<DATA, mgr_data>;
				using UpConv = std::is_convertible<mgr_data, DATA>;

				// static_assertion: DATAからMGR::data_typeは異なっていても良いが、アップコンバートかダウンコンバート出来なければならない
				struct Check {
					const int StaticAssertion[(DownConv::value || UpConv::value) ? 0 : -1];
				};
			}
			WHandleT(const WHandleT&) = default;
			// data_typeが異なっていてもアップコンバート可能ならば暗黙的な変換を許可する
			template <class DAT,
					class = typename std::enable_if<std::is_convertible<DAT, DATA>::value>::type>
			WHandleT(const WHandleT<MGR,DAT>& wh): WHandle(wh) {}
			WHandle getBase() const { return *this; }
			//! リソース参照
			/*!	参照が無効なら例外を投げる
			*	\return リソースの参照 */
			HdlLock<SHdl,true> lock() const { return MGR::_ref().lockLH(*this); }
			bool isHandleValid() const { return MGR::_ref().isHandleValid(*this); }
	};
	//! shared_ptrで言うenable_shared_from_thisの、リソースマネージャ版
	template <class SH>
	class EnableFromThis {
		using sh_t = SH;
		using mgr_t = typename sh_t::mgr_type;
		using data_t = typename sh_t::data_type;
		using wh_t = WHandleT<mgr_t, data_t>;
		using lh_t = HdlLock<sh_t, true>;
		template <class DATA, class DERIVED, template <class> class Allocator>
		friend class ResMgrA;
		private:
			wh_t	_wh_this;
			void _setResourceHandle(sh_t sh) {
				_wh_this = sh.weak();
			}
		public:
			using EnableFromThis_Tag = int;
			lh_t handleFromThis() const {
				lh_t ret(_wh_this.lock());
				AssertP(Trap, ret.valid())
				return ret;
			}
	};

	class URI;
	struct IURIOpener {
		virtual UP_Adapt openURI(const URI& uri) = 0;
		virtual ~IURIOpener() {}
	};
	using SP_URIOpener = std::shared_ptr<IURIOpener>;
	template <class T, class Chk, class Prio=uint32_t>
	class HandlerV {
		using SPHandler = std::shared_ptr<T>;
		using HandlerPair = std::pair<Prio, SPHandler>;
		using HandlerVec = std::vector<HandlerPair>;
		HandlerVec	_handler;
		static T*	s_dummy;
		typename HandlerVec::iterator _findHandler(const SPHandler& h) {
			return std::find_if(_handler.begin(), _handler.end(), [&h](const HandlerPair& p){
				return p.second == h;
			});
		}
		public:
			void addHandler(Prio prio, const SPHandler& h) {
				Assert(Trap, _findHandler(h)==_handler.end())
				_handler.emplace_back(prio, h);
				std::sort(_handler.begin(), _handler.end(), [](const HandlerPair& p0, const HandlerPair& p1) {
					return p0.first < p1.first; });
			}
			void remHandler(const SPHandler& h) {
				auto itr = _findHandler(h);
				Assert(Trap, itr!=_handler.end());
				_handler.erase(itr);
			}
			template <class... U>
			auto procHandler(U&&... u) const -> decltype(std::declval<Chk>()(*s_dummy,std::forward<U>(u)...)) {
				Chk chk;
				for(auto& p : _handler) {
					if(auto ret = chk(*p.second, std::forward<U>(u)...))
						return ret;
				}
				return decltype(std::declval<Chk>()(*s_dummy,std::forward<U>(u)...))();
			}
			void clearHandler() {
				_handler.clear();
			}
	};
	template <class T, class Chk, class Prio>
	T* HandlerV<T,Chk,Prio>::s_dummy(nullptr);

	using LHandle = HdlLock<SHandle,true>;
	//! 型を限定しないリソースマネージャ基底
	#define mgr_base	(::spn::ResMgrBase::_ref())
	class ResMgrBase {
		using RMList = noseq_list<ResMgrBase*, std::allocator, int>;
		static RMList						s_rmList;
		struct HChk {
			UP_Adapt operator()(IURIOpener& o, const URI& uri) const {
				return o.openURI(uri);
			}
		};
		using URIHandlerV = HandlerV<IURIOpener, HChk>;
		static URIHandlerV	s_handler;
		protected:
			int _addManager(ResMgrBase* p);
			void _remManager(int id);
		public:
			static URIHandlerV& GetHandler();
			// ハンドルのResIDから処理するマネージャを特定
			static void Increment(SHandle sh);
			static bool Release(SHandle sh);
			static uint32_t Count(SHandle sh);
			static SHandle Lock(WHandle wh);
			static WHandle Weak(SHandle sh);
			static ResMgrBase* GetManager(int rID);
			static void* GetPtr(SHandle sh);
			//! URLに対応したリソースマネージャを探し、ハンドルを生成
			static LHandle LoadResource(const URI& uri);

			virtual void increment(SHandle sh) = 0;
			virtual bool release(SHandle sh) = 0;
			virtual uint32_t count(SHandle sh) const = 0;
			virtual void* getPtr(SHandle sh) = 0;
			virtual SHandle lock(WHandle wh) = 0;
			virtual WHandle weak(SHandle sh) = 0;
			virtual const std::string& getResourceName(SHandle sh) const;
			virtual ~ResMgrBase() {}
			//! 対応する拡張子か判定し、もしそうなら読み込む
			virtual LHandle loadResource(AdaptStream& ast, const URI& uri);
	};

	//! 名前付きリソース管理の為のラッパ
	template <class T, class KEY>
	struct ResWrap {
		T 			value;
		const KEY*	stp;

		ResWrap(ResWrap&& r): value(std::move(r.value)), stp(r.stp) {}
		template <class... Ts>
		ResWrap(Ts&&... ts): value(std::forward<Ts>(ts)...), stp(nullptr) {}
		operator T&() { return value; }
		operator const T&() const { return value; }

		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			bool bStp = stp!=nullptr;
			ar & BOOST_SERIALIZATION_NVP(value) & BOOST_SERIALIZATION_NVP(bStp);
			if(!bStp)
				stp = nullptr;
		}
	};
	//! ResWrap<>を取り除く
	template <class T>
	struct DecayWrap {
		using type = T;
	};
	template <class T, class K>
	struct DecayWrap<ResWrap<T,K>> {
		using type = T;
	};
	//! 名前なしリソース (anonymous-only)
	template <class DAT, class DERIVED, template <class> class Allocator=std::allocator>
	class ResMgrA : public Singleton<DERIVED>, public ResMgrBase {
		public:
			using data_type = typename DecayWrap<DAT>::type;
			using ThisType = ResMgrA<DAT,DERIVED,Allocator>;
			using SHdl = SHandleT<ThisType>;
			using WHdl = WHandleT<ThisType>;
			using LHdl = HdlLock<SHdl,true>;
		public:
			class Entry : public boost::serialization::traits<Entry,
							boost::serialization::object_serializable,
							boost::serialization::track_never>
			{
				Entry() = default;
				friend _OptionalBuff<Entry>;
				friend boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int) {
					ar & BOOST_SERIALIZATION_NVP(data) & BOOST_SERIALIZATION_NVP(count) & BOOST_SERIALIZATION_NVP(accessCount) & BOOST_SERIALIZATION_NVP(w_magic);
					#ifdef DEBUG
						ar & BOOST_SERIALIZATION_NVP(magic);
					#endif
				}

				public:
				DAT						data;
				uint32_t				count,			//!< 参照カウンタ
										accessCount;	//!< refアクセス回数カウンタ
				typename WHdl::VWord	w_magic;

				template <class DAT2>
				Entry(DAT2&& dat, typename WHdl::VWord wmagic): data(std::forward<DAT2>(dat)), count(0), accessCount(0), w_magic(wmagic) {}
				#ifdef DEBUG
					typename SHdl::VWord	magic;
					// デバッグ版では簡易マジックナンバーチェックが入る
					template <class DAT2>
					Entry(DAT2&& dat, typename WHdl::VWord wmagic, typename SHdl::VWord smagic): Entry(std::forward<DAT2>(dat), wmagic) {
						magic = smagic;
					}
				#endif

				Entry(Entry&& e): data(std::forward<DAT>(e.data)), count(e.count), accessCount(e.accessCount), w_magic(e.w_magic)
				#ifdef DEBUG
					, magic(e.magic)
				#endif
				{}

				Entry& operator = (Entry&& e) {
					data = std::move(e.data);
					count = e.count;
					w_magic = e.w_magic;
					#ifdef DEBUG
						magic = e.magic;
					#endif
					return *this;
				}
				Entry& operator = (const Entry& e) = default;
				bool operator == (const Entry& e) const {
					return count == e.count &&
							accessCount == e.accessCount &&
							w_magic == e.w_magic &&
							#ifdef DEBUG
								magic == e.magic &&
							#endif
							data == e.data;
				}

				operator typename TheType<data_type>::type () { return data; }
				operator typename TheType<data_type>::ctype () const { return data; }
			};
		private:
			friend SHdl;
			friend WHdl;
			using AVec = noseq_list<Entry, Allocator, uint16_t, (1 << SHandle::Value::BFAt<SHandle::HandleBits::INDEX>::length)>;

			#ifdef DEBUG
				uint32_t	_sMagicIndex = 0;
			#endif
			uint64_t		_wMagicIndex = 0;
			AVec			_dataVec;
			int				_resID;
			//! シリアライズ中の時だけtrue
			/*! シリアライズ中はリソースの確保や解放、参照等のアクセスが不正 */
			mutable bool	_bSerializing = false;

			bool _checkValidAccess() const {
				return !_bSerializing;
			}
			friend boost::serialization::access;
			BOOST_SERIALIZATION_SPLIT_MEMBER();
			template <class Archive>
			void load(Archive& ar, const unsigned int ver) {
				using boost::serialization::make_nvp;
				AssertP(Trap, !_bSerializing)
				_bSerializing = true;

				// 最初にMergeフラグを読む
				bool bMerge;
				ar & make_nvp("mergeflag", bMerge);
				normal_serialize(ar, ver);

				if(bMerge) {
					// アクセスカウントとWeakマジックナンバー
					std::vector<std::pair<uint32_t, typename WHdl::VWord>>	id_list;
					ar & make_nvp("id_list", id_list);
					// 統合ロード(既に同じリソースが存在している場合(アクセスカウンタ一致)は読み込まない)
					auto nData = id_list.size();
					std::vector<bool> chk(nData);
					for(size_t i=0 ; i<nData ; i++) {
						auto& idl = id_list[i];
						bool flag = false;
						if(_dataVec.has(i)) {
							Entry& ent = _dataVec.get(i);
							if(ent.accessCount == idl.first) {
								if(ent.w_magic == idl.second)
									flag = true;
							}
						}
						chk[i] = flag;
					}
					_dataVec._load_partial(ar, chk);
				}
				// 全消しして上書き or IDのみ (フラグで判断)
				ar & BOOST_SERIALIZATION_NVP(_dataVec);

				// シリアライズフラグの解除は自動
				_bSerializing = false;
			}
			template <class Archive>
			void save(Archive& ar, const unsigned int ver) const {
				using boost::serialization::make_nvp;
				AssertP(Trap, !_bSerializing)
				_bSerializing = true;

				ar & make_nvp("mergeflag", s_bMerge);
				auto* ths = const_cast<ThisType*>(this);
				ths->normal_serialize(ar, ver);

				if(s_bMerge) {
					// 統合ロードの為にリソースハンドル毎に切り出して保存
					s_bMerge = false;
					size_t nData = _dataVec.size();
					// アクセスカウントとWeakマジックナンバーの出力
					{
						std::vector<std::pair<uint32_t, typename WHdl::VWord>>	tmp(nData);
						auto* pTmp = tmp.data();
						for(auto& e : _dataVec) {
							pTmp->first = e.accessCount;
							pTmp->second = e.w_magic;
							++pTmp;
						}
						ar & make_nvp("id_list", tmp);
					}

					// データ本体の出力
					{
						std::stringstream			ss;
						std::vector<ByteArray>		tmp(nData);
						for(size_t i=0 ; i<nData ; i++) {
							// 一旦ローカルに出力してから・・
							boost::archive::binary_oarchive oa(ss, boost::archive::no_header);
							oa & make_nvp("data", _dataVec.getValueOP(i));

							// 改めて外部に出力
							tmp[i] = ss.str();

							ss.str("");
							ss.clear();
							ss << std::dec;
						}
						ar & make_nvp("data", tmp);
					}
					// DataVecのIDSだけ出力
					ar & make_nvp("ids_list", _dataVec.asIDSType());
					s_bMerge = true;
				} else {
					// 普通に出力
					ar & BOOST_SERIALIZATION_NVP(_dataVec);
				}

				// シリアライズフラグの解除はマニュアル
			}
			template <class Archive>
			void normal_serialize(Archive& ar, const unsigned int /*ver*/) {
				#ifdef DEBUG
					ar & BOOST_SERIALIZATION_NVP(_sMagicIndex);
				#endif
				ar & BOOST_SERIALIZATION_NVP(_wMagicIndex) & BOOST_SERIALIZATION_NVP(_resID);
			}
			//! シリアライズの時の分岐
			static bool s_bMerge;
			//! シリアライズフラグ切り替え
			struct MergeType : boost::serialization::traits<MergeType,
					boost::serialization::object_serializable,
					boost::serialization::track_never>
			{
				const ThisType* _ths;

				BOOST_SERIALIZATION_SPLIT_MEMBER();
				// セーブ関数しか用意しない
				template <class Archive>
				void save(Archive& ar, const unsigned int) const {
					bool tmp = s_bMerge;
					s_bMerge = true;
					ar & (*_ths);
					s_bMerge = tmp;
				}
			};
			static MergeType s_merge;

		protected:
			/*! DEBUG時は簡易マジックナンバーのチェックをする */
			Entry& _refSH(SHdl sh) {
				const ThisType* ths = this;
				return const_cast<Entry&>(ths->_refSH(sh));
			}
			const Entry& _refSH(SHdl sh) const {
				AssertP(Trap, _checkValidAccess(), "accessing resource is invalid operation while serialize")
				auto& ent = _dataVec.get(sh.getIndex());
				#ifdef DEBUG
					AssertP(Trap, ent.magic==sh.getMagic(), "ResMgr: invalid magic number(Ent:%1% != Handle:%2%)", ent.magic, sh.getMagic())
				#endif
				return ent;
			}
			/*! 弱参照用のマジックナンバーチェック */
			spn::Optional<Entry&> _refWH(WHdl wh) {
				auto* ths = const_cast<const ResMgrA*>(this);
				if(auto op = ths->_refWH(wh))
					return const_cast<Entry&>(*op);
				return spn::none;
			}
			spn::Optional<const Entry&> _refWH(WHdl wh) const {
				AssertP(Trap, _checkValidAccess(), "accessing resource is invalid operation while serialize")
				auto idx = wh.getIndex();
				if(_dataVec.has(idx)) {
					auto& ent = _dataVec.get(idx);
					if(ent.w_magic == wh.getMagic())
						return ent;
				}
				// インデックスが既に無効 = オブジェクトは既に存在しない
				return spn::none;
			}

			template <class DAT2>
			LHdl _acquire(DAT2&& d) {
				decltype(auto) dat = spn::dereference(d);
				AssertP(Trap, _checkValidAccess(), "creating resource is invalid operation while serialize")
				++_wMagicIndex;
				_wMagicIndex &= WHandle::Value::length_mask<WHandle::Value::MAGIC>();
				#ifdef DEBUG
					++_sMagicIndex;
					_sMagicIndex &= SHandle::Value::length_mask<SHandle::Value::MAGIC>();
					auto id = _dataVec.add(Entry(std::forward<DAT2>(d), _wMagicIndex, _sMagicIndex));
					SHdl sh(id, _resID, _sMagicIndex);
				#else
					auto id = _dataVec.add(Entry(std::forward<DAT2>(d), _wMagicIndex));
					SHdl sh(id, _resID);
				#endif
				// EnableFromThisを継承したクラスの場合はここでハンドルをセットする
				_setFromThis(sh, dat, 0);
				return LHdl(sh);
			}
			//! EnableFromThisを継承しているか
			template <class D>
			void _setFromThis(SHdl sh, D& d, typename dereference_ptr_t<D>::EnableFromThis_Tag) {
				dereference(d)._setResourceHandle(sh);
			}
			template <class D>
			void _setFromThis(SHdl, D&, ...) {}
		public:
			const MergeType& asMergeType() const {
				s_merge._ths = this;
				return s_merge; }
			class iterator : public AdaptItrBase<data_type, typename AVec::iterator> {
				public:
					using Itr = AdaptItrBase<data_type, typename AVec::iterator>;
					using Itr::Itr;
			};
			class const_iterator : public AdaptItrBase<const data_type, typename AVec::const_iterator> {
				public:
					using Itr = AdaptItrBase<const data_type, typename AVec::const_iterator>;
					using Itr::Itr;
			};
			class reverse_iterator : public AdaptItrBase<data_type, typename AVec::reverse_iterator> {
				public:
					using Itr = AdaptItrBase<data_type, typename AVec::reverse_iterator>;
					using Itr::Itr;
			};
			class const_reverse_iterator : public AdaptItrBase<const data_type, std::reverse_iterator<typename AVec::const_iterator>> {
				public:
					using Itr = AdaptItrBase<const data_type, std::reverse_iterator<typename AVec::const_iterator>>;
					using Itr::Itr;
			};

			BOOST_PP_REPEAT(16, ITERATOR_ADAPTOR, _dataVec)

			ResMgrA() {
				// リポジトリへ登録
				_resID = _addManager(this);
			}
			~ResMgrA() {
				// 全ての現存リソースに対して1回ずつデクリメントすれば全て解放される筈
				int count = 0;
				_dataVec.iterate([&count](Optional<Entry>& ent){
					if(--(*ent).count == 0) {
						// オブジェクトの解放
						// 巡回中はエントリを消すと不具合が出るのでデストラクタを呼ぶのみ
						ent = none;
						++count;
					}
				});
				// 明示的に解放されてないリソースがある場合は警告を出す
				int remain = static_cast<int>(_dataVec.size()) - count;
				if(remain > 0)
					std::cerr << "ResMgr: there are some unreleased resources...(remaining " << remain << ')' << std::endl;

				_remManager(_resID);
			}
			auto getResourceId() const {
				return _resID;
			}
			//! シリアライズフラグをリセット
			void resetSerializeFlag() {
				AssertP(Trap, _bSerializing, "serialize flag has already reset")
				_bSerializing = false;
			}
			//! 参照カウンタをインクリメント
			void increment(SHandle sh) override {
				Entry& ent = _refSH(sh);
				++ent.count;
			}
			void* getPtr(SHandle sh) override {
				auto& ent = _refSH(sh);
				return &ent.data;
			}
			//! 名前なしリソースの作成
			template <class DAT2>
			LHdl acquire(DAT2&& dat) {
				return _acquire(std::forward<DAT2>(dat));
			}
			template <class... Ts>
			LHdl emplace(Ts&&... args) {
				return _acquire(DAT(std::forward<Ts>(args)...));
			}
			// データが非スマートポインタの場合はemplaceと同じ
			template <class D>
			struct CallAcquire {
				template <class T, class MGR, class... Ts>
				static auto Proc(MGR& m, Ts&&... args) {
					return m._acquire(T(std::forward<Ts>(args)...));
				}
			};
			// AlignedNewしたポインタをacquireして返す (unique_ptr用)
			template <class... Ds>
			struct CallAcquire<std::unique_ptr<Ds...>> {
				template <class T, class MGR, class... Ts>
				static auto Proc(MGR& m, Ts&&... args) {
					return m._acquire(AAllocator<T>::template NewUF(std::forward<Ts>(args)...));
				}
			};
			// AlignedNewしたポインタをacquireして返す (shared_ptr用)
			template <class D>
			struct CallAcquire<std::shared_ptr<D>> {
				template <class T, class MGR, class... Ts>
				static auto Proc(MGR& m, Ts&&... args) {
					return m._acquire(AAllocator<T>::NewS(std::forward<Ts>(args)...));
				}
			};
			//! 任意の引数からリソースハンドル作成
			template <class TF=DAT, class... Ts>
			LHdl makeHandle(Ts&&... args) {
				return CallAcquire<DAT>::template Proc<TF>(*this, std::forward<Ts>(args)...);
			}
			const static std::function<void (Entry&)> cs_defCB;
			template <class CB>
			bool releaseWithCallback(SHandle sh, CB cb=cs_defCB) {
				auto& ent = _refSH(sh);
				// 簡易マジックナンバーチェック
				AssertP(Trap, ent.magic == sh.getMagic(), "ResMgr: invalid magic number(Ent:%1% != Handle:%2%)", ent.magic, sh.getMagic())
				if(--ent.count == 0) {
					cb(ent);
					// オブジェクトの解放
					_dataVec.rem(sh.getIndex());
					return true;
				}
				return false;
			}
			bool release(SHandle sh) override {
				return releaseWithCallback(sh, [](Entry&){});
			}
			WHandle weak(SHandle sh) override {
				auto& ent = _refSH(sh);
				return WHandle(sh.getIndex(), sh.getResID(), ent.w_magic);
			}
			SHandle lock(WHandle wh) override {
				auto opt = _refWH(wh);
				if(opt) {
					// 参照カウントインクリメント
					++opt->count;
					return SHandle(wh.getIndex(), wh.getResID()
					#ifdef DEBUG
						, opt->magic
					#endif
					);
				}
				return SHandle();
			}
			//! 弱参照ハンドルから実体を得る
			LHdl lockLH(WHandle wh) {
				return LHdl(lock(wh), LHdl::AsLocked);
			}
			//! 弱参照ハンドルが有効かどうかを判定
			bool isHandleValid(WHandle wh) const {
				return _refWH(wh);
			}
			//! 参照数を得る
			uint32_t count(SHandle sh) const override {
				auto& ent = _refSH(sh);
				return ent.count;
			}
			//! アクセスカウントを得る(変更のチェック用)
			uint32_t accessCount(SHandle sh) const {
				auto& ent = _refSH(sh);
				return ent.accessCount;
			}
			data_type& ref(SHandle sh) {
				auto& ent = _refSH(sh);
				// 非constアクセス時はアクセスカウンタの加算
				++ent.accessCount;
				return ent.data;
			}
			const data_type& cref(SHandle sh) const {
				const auto& ent = _refSH(sh);
				return ent.data;
			}
			//! 同じリソース機構を使用する別のハンドル型を生成
			template <class NDATA>
			using AnotherSHandle = SHandleT<ThisType, NDATA>;
			template <class NDATA>
			using AnotherWHandle = WHandleT<ThisType, NDATA>;
			template <class NDATA, bool DIRECT>
			using AnotherLHandle = HdlLock<AnotherSHandle<NDATA>, DIRECT>;
			size_t size() const {
				return _dataVec.size();
			}
			//! 主にデバッグ用途
			AVec getNSeq() {
				return std::move(_dataVec);
			}

		protected:
			//! 継承先クラスにて内部データ型をダウンキャストする際に使用
			template <class NDATA, class DATA>
			static AnotherSHandle<NDATA> Cast(AnotherSHandle<DATA>&& h) {
				// Handleのprivateなコンストラクタを経由して変換
				return AnotherSHandle<NDATA>(std::move(h)); }
			template <class NDATA, class DATA>
			static AnotherSHandle<NDATA> Cast(const AnotherSHandle<DATA>& h) {
				return AnotherSHandle<NDATA>(h); }

			template <class NDATA, template<class,bool> class HL, class DATA, bool DIRECT>
			static HL<AnotherSHandle<NDATA>,DIRECT> Cast(HL<AnotherSHandle<DATA>,DIRECT>&& lh) {
				return HL<AnotherSHandle<NDATA>,DIRECT>(std::move(lh)); }
			template <class NDATA, template<class,bool> class HL, class DATA, bool DIRECT>
			static HL<AnotherSHandle<NDATA>,DIRECT> Cast(const HL<AnotherSHandle<DATA>,DIRECT>& lh) {
				return HL<AnotherSHandle<NDATA>,DIRECT>(lh); }
	};
	template <class DAT, class DERIVED, template <class> class Allocator>
	const std::function<void (typename ResMgrA<DAT,DERIVED,Allocator>::Entry&)> ResMgrA<DAT,DERIVED,Allocator>::cs_defCB = [](Entry&){};
	template <class DAT, class DERIVED, template <class> class Allocator>
	bool ResMgrA<DAT,DERIVED,Allocator>::s_bMerge = false;
	template <class DAT, class DERIVED, template <class> class Allocator>
	typename ResMgrA<DAT,DERIVED,Allocator>::MergeType ResMgrA<DAT,DERIVED,Allocator>::s_merge{};

	//! シリアライズ対策の為のstd::stringラッパ
	/*! boostデフォルトのstd::string定義ではprimitive-typeになっていて同じ文字列が複数回シリアライズされてしまうのでその対策 */
	template <class T>
	class String : public T {
		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & boost::serialization::make_nvp("string", static_cast<T&>(*this));
		}
		public:
			String() = default;
			String(String&& s): T(std::move(static_cast<T&>(s))) {}
			String(const String& s): T(static_cast<const T&>(s)) {}
			String(const T& t): T(t) {}
			String(T&& t): T(std::move(t)) {}
			using T::T;
	};
	//! 名前付きリソース (with anonymous)
	template <class DAT, class DERIVED, template <class> class Allocator=std::allocator, class KEY=String<std::string>>
	class ResMgrN : public ResMgrA<ResWrap<DAT,KEY>, DERIVED, Allocator> {
		public:
			using base_type = ResMgrA<ResWrap<DAT,KEY>, DERIVED, Allocator>;
			using LHdl = typename base_type::LHdl;
			using SHdl = typename base_type::SHdl;
			using key_t = KEY;
		private:
			using NameMap = std::unordered_map<KEY, typename base_type::SHdl>;
			NameMap		_nameMap;

			//! リソースの新規作成。既に存在する場合は置き換え
			/*! \param key		置き換え対象のリソースに対応するキー
				\param cb		リソースを作成する関数オブジェクト
				\return			新たに作成されたリソースハンドル
			*/
			template <class CB>
			LHdl _replace(key_t&& key, CB&& cb) {
				auto key_c = _modifyResourceName(key);
				auto itr = _nameMap.find(key_c);
				if(itr != _nameMap.end()) {
					// 古いリソースは名前との関連付けを外す
					SHdl h = itr->second;
					base_type::_refSH(h).data.stp = nullptr;
					// 古いハンドルの所有権は外部が持っているのでここでは何もしない

					auto lh = std::forward<CB>(cb)(std::move(key));
					itr->second = lh.get();
					base_type::_refSH(lh.get()).data.stp = &(itr->first);
					// 名前ポインタはそのまま流用
					return lh;
				}
				// エントリの新規作成
				auto lh = std::forward<CB>(cb)(std::move(key));
				itr = _nameMap.insert(std::make_pair(std::move(key_c), lh.get())).first;
				// 名前登録
				auto& ent = base_type::_refSH(lh.get());
				ent.data.stp = &itr->first;
				return lh;
			}
			//! リソースの新規作成。既に存在する場合はそれを返す
			/*! \param key		置き換え対象のリソースに対応するキー
				\param cb		リソースを作成する関数オブジェクト (新規作成される場合のみ使用)
				\return			(リソースハンドル, 新規作成フラグ)
			*/
			template <class CB>
			std::pair<LHdl,bool> _acquire(key_t&& key, CB&& cb) {
				auto key_c = _modifyResourceName(key);
				// 既に同じ名前が登録されていたら既存のハンドルを返す
				auto itr = _nameMap.find(key_c);
				if(itr != _nameMap.end())
					return std::make_pair(LHdl(itr->second), false);

				auto lh = std::forward<CB>(cb)(std::move(key));
				itr = _nameMap.emplace(std::move(key_c), lh.get()).first;
				auto& ent = base_type::_refSH(lh.get());
				ent.data.stp = &(itr->first);
				return std::make_pair(std::move(lh), true);
			}
			//! リソース名修正関数
			/*! 必要に応じてリソース名の加工を行う。
				戻り値がキーとして使用され、引数はコールバック関数へ渡される */
			virtual key_t _modifyResourceName(key_t& key) const { return key; }

			BOOST_SERIALIZATION_SPLIT_MEMBER();
			friend boost::serialization::access;
			template <class Archive>
			void load(Archive& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(_nameMap) & BOOST_SERIALIZATION_BASE_OBJECT_NVP(base_type);
				// ポインタの振りなおし
				for(auto& p : _nameMap) {
					auto& data = base_type::_refSH(p.second).data;
					data.stp = &p.first;
				}
			}
			template <class Archive>
			void save(Archive& ar, const unsigned int /*ver*/) const {
				ar & BOOST_SERIALIZATION_NVP(_nameMap) & BOOST_SERIALIZATION_BASE_OBJECT_NVP(base_type);
			}

		public:
			using base_type::acquire;

			const KEY& getKey(SHdl sh) const {
				// とりあえず線形探索で実装
				for(auto& ent : _nameMap) {
					if(ent.second == sh) {
						return ent.first;
					}
				}
				Assert(Trap, false, "resource key not found")
				throw 0;
			}
			//! 同じ要素が存在したら置き換え
			template <class KEY2, class DATA>
			LHdl replace(KEY2&& key, DATA&& dat) {
				auto fn = [&](key_t&&){ return base_type::acquire(std::forward<DATA>(dat)); };
				return _replace(key_t(std::forward<KEY2>(key)), fn);
			}
			//! 名前付きリソースの作成
			/*! \return [リソースハンドル,
			 *			新たにエントリが作成されたらtrue, 既存のキーが使われたらfalse] */
			template <class KEY2, class CB>
			std::pair<LHdl,bool> acquire(KEY2&& key, CB&& cb) {
				auto fn = [&](const auto& key){ return base_type::acquire(std::forward<CB>(cb)(key)); };
				return _acquire(key_t(std::forward<KEY2>(key)), fn);
			}
			//! キーを指定してハンドルを取得
			LHdl getFromKey(const KEY& key) const {
				auto itr = _nameMap.find(key);
				if(itr != _nameMap.end())
					return itr->second;
				return LHdl();
			}
			bool release(SHandle sh) override {
				auto& ent = base_type::_refSH(SHdl::FromHandle(sh));
				const KEY* stp = ent.data.stp;
				if(stp) {
					return base_type::releaseWithCallback(sh, [this, stp](typename base_type::Entry&){
						// 名前も消す
						_nameMap.erase(_nameMap.find(*stp));
					});
				}
				return base_type::release(sh);
			}
	};
	// ------ 互換ハンドル一式を定義するマクロ ------
	//! 名前なしハンドル
	/*! \param mgr	マネージャクラス名
		\param suffix	ハンドルに付けるサフィックス
		\param bsp	元のデータ型
		\param sp	参照時のデータ型
		\param alloc	マネージャが使用するアロケータ */
	#define DEF_AHANDLE_PROP(mgr, suffix, bsp, sp, alloc) \
		using H##suffix = spn::SHandleT<spn::ResMgrA<bsp, mgr, alloc>, sp>; \
		using W##suffix = spn::WHandleT<spn::ResMgrA<bsp, mgr, alloc>, sp>; \
		using HL##suffix = spn::HdlLock<H##suffix, true>; \
		using BOOST_PP_CAT(BOOST_PP_CAT(HL,suffix),F) = spn::HdlLock<H##suffix, false>;
	#define DEF_AHANDLE(mgr, suffix, bsp, sp) DEF_AHANDLE_PROP(mgr, suffix, bsp, sp, std::allocator)
	//! 名前付きハンドル
	/*! \param key	キーの型 */
	#define DEF_NHANDLE_PROP(mgr, suffix, bsp, sp, alloc, key) \
		using H##suffix = spn::SHandleT<spn::ResMgrA<spn::ResWrap<bsp,key>, mgr, alloc>, sp>; \
		using W##suffix = spn::WHandleT<spn::ResMgrA<spn::ResWrap<bsp,key>, mgr, alloc>, sp>; \
		using HL##suffix = spn::HdlLock<H##suffix, true>; \
		using BOOST_PP_CAT(BOOST_PP_CAT(HL,suffix),F) = spn::HdlLock<H##suffix, false>;
	#define DEF_NHANDLE(mgr ,suffix, bsp, sp) DEF_NHANDLE_PROP(mgr, suffix, bsp, sp, std::allocator, spn::String<std::string>)

	//! SHandleT, WHandleT, HdlLockの時true_type, それ以外はfalse_type
	template <class T>
	struct IsHandleT : std::false_type {};
	template <class Mgr, class Data>
	struct IsHandleT<SHandleT<Mgr,Data>> : std::true_type {};
	template <class Mgr, class Data>
	struct IsHandleT<WHandleT<Mgr,Data>> : std::true_type {};
	template <class H, bool B>
	struct IsHandleT<HdlLock<H,B>> : std::true_type {};

	//! IsHandleT | SHandle | WHandle の時true_type, それ以外はfalse_type
	template <class T>
	struct IsHandle : IsHandleT<T> {};
	template <>
	struct IsHandle<SHandle> : std::true_type {};
	template <>
	struct IsHandle<WHandle> : std::true_type {};
}
namespace std {
	template <class MGR, class DATA>
	struct hash<spn::SHandleT<MGR,DATA>> {
		size_t operator()(const spn::SHandle& sh) const {
			return sh.getValue();
		}
	};
	template <class MGR, class DATA>
	struct hash<spn::WHandleT<MGR,DATA>> {
		size_t operator()(const spn::WHandle& wh) const {
			return hash<uint64_t>()(wh.getValue());
		}
	};
	template <class H, bool D>
	struct hash<spn::HdlLock<H,D>> {
		size_t operator()(const spn::HdlLock<H,D>& h) const {
			return hash<H>()(h.get());
		}
	};
}


BOOST_CLASS_IMPLEMENTATION(spn::SHandle, object_serializable)
BOOST_CLASS_TRACKING(spn::SHandle, track_never)
BOOST_CLASS_IMPLEMENTATION(spn::WHandle, object_serializable)
BOOST_CLASS_TRACKING(spn::WHandle, track_never)
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class), spn::SHandleT, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class), spn::SHandleT, track_never)
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class), spn::WHandleT, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class), spn::WHandleT, track_never)
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class)(bool), spn::HdlLockB, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class)(bool), spn::HdlLockB, track_never)

BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class)(class), spn::ResWrap, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class)(class), spn::ResWrap, track_selectively)
BOOST_CLASS_VERSION_TEMPLATE((class)(class)(template<class> class), spn::ResMgrA, 0)
BOOST_CLASS_VERSION_TEMPLATE((class)(class)(template<class> class)(class), spn::ResMgrN, 0)

BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class), spn::String, object_serializable)

template <class T>
inline bool operator == (const spn::String<T>& s0, const T& s1) {
	return s0 == s1; }
template <class T>
inline bool operator == (const T& s0, const spn::String<T>& s1) {
	return s0 == s1; }
namespace std {
	//MEMO: 部分特殊化は宜しくないと聞いた気がするが他に思い浮かばないのでとりあえずコレで行く
	template <class T>
	struct hash<spn::String<T>> {
		size_t operator()(const spn::String<T>& str) const {
			return hash<T>()(str);
		}
	};
}
