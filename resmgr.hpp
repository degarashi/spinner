#pragma once
#include "misc.hpp"
#include "bits.hpp"
#include "macro.hpp"
#include "noseq.hpp"
#include <unordered_map>
#include <string>

namespace spn {
	//! 型を保持しない強参照ハンドル値
	/*! インデックスに14bit, リソースIDに6bit, (デバッグ用)マジックナンバーに12bitを使用 */
	class SHandle {
		#ifdef DEBUG
			// デバッグ時のみ簡易マジックナンバーを保持
			struct MyDef : BitDef<uint32_t, BitF<0,14>, BitF<14,6>, BitF<20,12>> {
				enum { INDEX, RESID, MAGIC }; };
		#else
			struct MyDef : BitDef<uint32_t, BitF<0,14>, BitF<14,6>> {
				enum { INDEX, RESID }; };
		#endif
		public:
			using Value = BitField<MyDef>;
			using VWord = Value::Word;
		private:
			Value _value;
		public:
			//! デフォルト値は無効なハンドルID
			SHandle();
			SHandle(const SHandle& sh) = default;
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
			operator bool () const;
			bool operator == (const SHandle& sh) const;
			void swap(SHandle& sh) noexcept;
			/*! ResourceIDから対応マネージャを特定して解放 */
			void release();
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
		public:
			//! デフォルト値は無効なハンドルID
			WHandle();
			WHandle(const WHandle& wh) = default;
			WHandle(int idx, int resID, VWord mag);

			VWord getResID() const;
			VWord getIndex() const;
			VWord getMagic() const;
			VWord getValue() const;
			bool valid() const;
			void  swap(WHandle& wh) noexcept;
			operator bool () const;
			bool operator == (const WHandle& sh) const;
	};

	template <class MGR, class DATA>
	class SHandleT;
	template <class MGR, class DATA>
	class WHandleT;

	//! 強参照スマートハンドル
	template <class HDL>
	class HdlLock {
		HDL _hdl;
		#define DOWNCONV	typename std::enable_if<std::is_convertible<typename HDL::data_type, DATA>::value>::type
		#define UPCONV		typename std::enable_if<std::is_convertible<DATA,typename HDL::data_type>::value>::type
		#define SAMETYPE	typename std::enable_if<!std::is_same<DATA, typename HDL::data_type>::value>::type

		using data_type = typename HDL::data_type;
		using WHdl = typename HDL::WHdl;
		friend typename HDL::mgr_type;
		private:
			//! 参照インクリメント(down convert)
			template <class DATA, class = DOWNCONV, class = SAMETYPE>
			HdlLock(const HdlLock<SHandleT<typename HDL::mgr_type, DATA>>& hl): HdlLock(hl.get()) {}
			template <class DATA, class = DOWNCONV, class = SAMETYPE>
			HdlLock(HdlLock<SHandleT<typename HDL::mgr_type, DATA>>&& hl) {
				//WARN: メモリを直接読み込んでしまっている、スマートじゃない実装
				_hdl.swap(*(HDL*)&hl);
			}

		public:
			HdlLock() {}
			HdlLock(const HdlLock& hdl): _hdl(hdl.get()) {
				if(_hdl)
					_hdl.increment();
			}
			//! 参照ムーブ(カウント変更なし)
			template <class DATA, class = UPCONV>
			HdlLock(HdlLock<SHandleT<typename HDL::mgr_type, DATA>>&& hl) noexcept {
				//WARN: メモリを直接読み込んでしまっている、スマートじゃない実装
				_hdl.swap(*(HDL*)&hl);
			}
			//! 参照インクリメント(up convert)
			template <class DATA, class = UPCONV>
			HdlLock(const HdlLock<SHandleT<typename HDL::mgr_type, DATA>>& hl): HdlLock(hl.get()) {}

			#undef DOWNCONV
			#undef UPCONV
			#undef SAMETYPE

			//! 参照インクリメント
			HdlLock(HDL hdl): _hdl(hdl) {
				if(_hdl.valid())
					hdl.increment();
			}
			HDL get() const { return _hdl; }
			~HdlLock() {
				release();
			}
			void release() {
				if(_hdl.valid()) {
					_hdl.release();
					_hdl = HDL();
				}
			}
			void reset(HDL hdl) {
				release();
				HdlLock tmp(hdl);
				swap(tmp);
			}
			void swap(HdlLock& hl) noexcept { _hdl.swap(hl._hdl); }
			HdlLock& operator = (const HdlLock& hl) {
				reset(hl.get());
				return *this;
			}
			HdlLock& operator = (HdlLock&& hl) noexcept {
				swap(hl);
				return *this;
			}
			bool operator == (const HdlLock& hl) const {
				return _hdl == hl.get();
			}

			operator bool () const { return valid(); }

			// ---- SHandleのメソッドを仲立ち ----
			data_type& ref() { return _hdl.ref(); }
			const data_type& cref() const { return _hdl.cref(); }
			bool valid() const { return _hdl.valid(); }
			WHdl weak() const { return _hdl.weak(); }
			uint32_t count() const { return _hdl.count(); }

			// ---- データにダイレクトアクセス ----
			const data_type* operator -> () const { return &_hdl.cref(); }
			data_type* operator -> () { return &_hdl.ref(); }
	};

	//! 強参照ハンドル
	template <class MGR, class DATA = typename MGR::data_type>
	class SHandleT : public SHandle {
		using mgr_data = typename MGR::data_type;
		using DownConv = std::is_convertible<mgr_data, DATA>;
		using UpConv = std::is_convertible<DATA, mgr_data>;

		// static_assertion: DATAからMGR::data_typeは異なっていても良いが、アップコンバートかダウンコンバート出来なければならない
		constexpr static int StaticAssertion[(DownConv::value || UpConv::value) ? 0 : -1] = {};

		friend MGR;
		friend class HdlLock<SHandleT>;
		private:
			// データ型をダウンコンバートする場合はMGRからしか許可しない
			template <class DAT, class = typename std::enable_if<DownConv::value>::type,
								class = typename std::enable_if<!std::is_same<DATA,mgr_data>::value>::type>
			SHandleT(const SHandleT<MGR,DAT>& sh): SHandle(sh) {}

		public:
			using WHdl = WHandleT<MGR, DATA>;
			using mgr_type = MGR;
			using data_type = DATA;
			using SHandle::SHandle;
			SHandleT() = default;
			SHandleT(const SHandle& hdl): SHandle(hdl) {}
			// data_typeが異なっていてもManagerが同じでMGR::data_typeへアップコンバート可能ならば暗黙的な変換を許可する
			template <class DAT, class = typename std::enable_if<UpConv::value>::type>
			SHandleT(const SHandleT<MGR, DAT>& hdl): SHandle(hdl) {}

			void release() { MGR::_ref().release(*this); }
			void increment() { MGR::_ref().increment(*this); }
			data_type& ref() {
				// Managerの保管している型とこのハンドルが返す型が異なる場合があるので明示的にキャストする
				return reinterpret_cast<data_type&>(MGR::_ref().ref(*this));
			}
			const data_type& cref() const {
				// ref()と同じ理由でキャスト
				return reinterpret_cast<const data_type&>(MGR::_ref().cref(*this));
			}
			//! 弱参照ハンドルを得る
			WHdl weak() const { return MGR::_ref().weak(*this); }
			uint32_t count() const { return MGR::_ref().count(*this); }

			operator bool () const { return valid(); }
			data_type* operator -> () { return &ref(); }
			const data_type* operator -> () const { return &cref(); }
	};
	//! 弱参照ハンドル
	template <class MGR, class DATA = typename MGR::data_type>
	class WHandleT : public WHandle {
		using mgr_data = typename MGR::data_type;
		using DownConv = std::is_convertible<mgr_data, DATA>;
		using UpConv = std::is_convertible<DATA, mgr_data>;

		// static_assertion: DATAからMGR::data_typeは異なっていても良いが、アップコンバートかダウンコンバート出来なければならない
		constexpr static int StaticAssertion[(DownConv::value || UpConv::value) ? 0 : -1] = {};

		friend MGR;
		private:
			// データ型をダウンコンバートする場合はMGRからしか許可しない
			template <class DAT, class = typename std::enable_if<DownConv::value>::type,
								class = typename std::enable_if<!std::is_same<DATA,mgr_data>::value>::type>
			WHandleT(const WHandleT<MGR,DAT>& wh): WHandle(wh) {}
		public:
			using SHdl = SHandleT<MGR, DATA>;
			using mgr_type = MGR;
			using data_type = DATA;
			using WHandle::WHandle;
			WHandleT() = default;
			WHandleT(const WHandle& wh): WHandle(wh) {}
			// data_typeが異なっていてもManagerが同じでMGR::data_typeへアップコンバート可能ならば暗黙的な変換を許可する
			template <class DAT, class = typename std::enable_if<UpConv::value>::type>
			WHandleT(const WHandleT<MGR,DAT>& wh): WHandle(wh) {}

			//! リソース参照
			/*!	参照が無効なら例外を投げる
			*	\return リソースの参照 */
			HdlLock<SHdl> lock() const { return MGR::_ref().lockLH(*this); }
			bool isHandleValid() const { return MGR::_ref().isHandleValid(*this); }
	};

	//! 型を限定しないリソースマネージャ基底
	class ResMgrBase {
		using RMList = noseq_list<ResMgrBase*, int>;
		static RMList s_rmList;
		protected:
			int _addManager(ResMgrBase* p);
			void _remManager(int id);
		public:
			// ハンドルのResIDから処理するマネージャを特定
			static void Increment(SHandle sh);
			static bool Release(SHandle sh);
			static SHandle Lock(WHandle wh);
			static WHandle Weak(SHandle sh);

			virtual void increment(SHandle sh) = 0;
			virtual bool release(SHandle sh) = 0;
			virtual SHandle lock(WHandle wh) = 0;
			virtual WHandle weak(SHandle sh) = 0;
			virtual ~ResMgrBase() {}
	};
	//! 名前付きリソース管理の為のラッパ
	template <class T>
	struct ResWrap {
		T 					value;
		const std::string*	stp;

		ResWrap(T&& t): value(std::forward<T>(t)), stp(nullptr) {}
		operator T&() { return value; }
		operator const T&() const { return value; }
	};
	//! ResWrap<>を取り除く
	template <class T>
	struct DecayWrap {
		using result = T;
	};
	template <class T>
	struct DecayWrap<ResWrap<T>> {
		using result = T;
	};
	//! 名前なしリソース (anonymous-only)
	template <class DAT, class DERIVED>
	class ResMgrA : public Singleton<DERIVED>, public ResMgrBase {
		public:
			using data_type = typename DecayWrap<DAT>::result;
			using ThisType = ResMgrA<DAT,DERIVED>;
			using SHdl = SHandleT<ThisType>;
			using WHdl = WHandleT<ThisType>;
			using LHdl = HdlLock<SHdl>;
		protected:
			struct Entry {
				DAT						data;
				uint32_t				count;
				typename WHdl::VWord	w_magic;

				Entry(DAT&& dat, typename WHdl::VWord wmagic): data(std::forward<DAT>(dat)), count(0), w_magic(wmagic) {}
				#ifdef DEBUG
					typename SHdl::VWord	magic;
					// デバッグ版では簡易マジックナンバーチェックが入る
					Entry(DAT&& dat, typename WHdl::VWord wmagic, typename SHdl::VWord smagic): Entry(std::forward<DAT>(dat), wmagic) {
						magic = smagic;
					}
				#endif

				Entry(Entry&& e): data(std::forward<DAT>(e.data)), count(e.count), w_magic(e.w_magic)
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

				operator typename TheType<data_type>::type () { return data; }
				operator typename TheType<data_type>::ctype () const { return data; }
			};
		private:
			friend SHdl;
			friend WHdl;
			using AVec = noseq_list<Entry, uint16_t>;

			#ifdef DEBUG
				uint32_t	_sMagicIndex = 0;
			#endif
			uint64_t		_wMagicIndex = 0;
			AVec			_dataVec;
			int				_resID;

		protected:
			/*! DEBUG時は簡易マジックナンバーのチェックをする */
			Entry& _refSH(SHdl sh) {
				Entry& ent = _dataVec.get(sh.getIndex());
				#ifdef DEBUG
					if(ent.magic != sh.getMagic())
						throw std::runtime_error("ResMgr: invalid magic number");
				#endif
				return ent;
			}
			const Entry& _refSH(SHdl sh) const {
				auto* ths = const_cast<ResMgrA*>(this);
				return ths->_refSH(sh);
			}
			/*! 弱参照用のマジックナンバーチェック */
			boost::optional<Entry&> _refWH(WHdl wh) {
				try {
					Entry& ent = _dataVec.get(wh.getIndex());
					if(ent.w_magic == wh.getMagic())
						return ent;
				} catch(const boost::bad_get& e) {
					// インデックスが既に無効 = オブジェクトは既に存在しない
				}
				return boost::none;
			}
			boost::optional<const Entry&> _refWH(WHdl wh) const {
				auto* ths = const_cast<ResMgrA*>(this);
				if(auto opt = ths->_refWH(wh))
					return *opt;
				return boost::none;
			}

		public:
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
						ent = boost::none;
						++count;
					}
				});
				// 明示的に解放されてないリソースがある場合は警告を出す
				int remain = static_cast<int>(_dataVec.size()) - count;
				if(remain > 0)
					std::cerr << "ResMgr: there are some unreleased resources...(remaining " << remain << ')' << std::endl;

				_remManager(_resID);
			}
			//! 参照カウンタをインクリメント
			void increment(SHandle sh) override {
				Entry& ent = _refSH(sh);
				++ent.count;
			}
			//! 名前なしリソースの作成
			template <class DAT2>
			LHdl acquire(DAT2&& dat) {
				#ifdef DEBUG
					auto id = _dataVec.add(Entry(std::forward<DAT2>(dat), ++_wMagicIndex, ++_sMagicIndex));
					_sMagicIndex &= SHandle::Value::length_mask<SHandle::Value::MAGIC>();
					SHdl sh(id, _resID, _sMagicIndex);
				#else
					auto id = _dataVec.add(Entry(std::forward<DAT2>(dat), ++_wMagicIndex));
					SHdl sh(id, _resID);
				#endif
				_wMagicIndex &= WHandle::Value::length_mask<WHandle::Value::MAGIC>();
				return LHdl(sh);
			}
			template <class CB>
			bool releaseWithCallback(SHandle sh, CB cb=[](Entry&){}) {
				auto& ent = _refSH(sh);
				#ifdef DEBUG
					// 簡易マジックナンバーチェック
					if(ent.magic != sh.getMagic())
						throw std::runtime_error("invalid magic number");
				#endif
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
				SHandle sh = lock(wh);
				if(sh.valid())
					release(sh);
				return LHdl(sh);
			}
			//! 弱参照ハンドルが有効かどうかを判定
			bool isHandleValid(WHandle wh) const {
				return _refWH(wh);
			}
			//! 参照数を得る
			uint32_t count(SHandle sh) const {
				auto& ent = _refSH(sh);
				return ent.count;
			}
			data_type& ref(SHandle sh) {
				auto& ent = _refSH(sh);
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
			template <class NDATA>
			using AnotherLHandle = HdlLock<AnotherSHandle<NDATA>>;

			//! 継承先クラスにて内部データ型をダウンキャストする際に使用
			template <class NDATA, template<class,class> class Handle, class DATA>
			static Handle<ThisType,NDATA> Cast(Handle<ThisType, DATA>&& h) {
				// Handleのprivateなコンストラクタを経由して変換
				return Handle<ThisType,NDATA>(std::forward<Handle<ThisType, DATA>>(h));
			}
			template <class NDATA, template<class> class HL, class DATA>
			static HL<AnotherSHandle<NDATA>> Cast(HL<AnotherSHandle<DATA>>&& lh) {
				return HL<AnotherSHandle<NDATA>>(std::forward<HL<AnotherSHandle<DATA>>>(lh));
			}
	};
	//! 名前付きリソース (with anonymous)
	template <class DAT, class DERIVED>
	class ResMgrN : public ResMgrA<ResWrap<DAT>, DERIVED> {
		public:
			using base_type = ResMgrA<ResWrap<DAT>, DERIVED>;
			using LHdl = typename base_type::LHdl;
			using SHdl = typename base_type::SHdl;
		private:
			using NameMap = std::unordered_map<std::string, typename base_type::SHdl>;
			NameMap		_nameMap;

			template <class KEY, class CB>
			LHdl _replace(KEY&& key, CB cb) {
				auto itr = _nameMap.find(key);
				if(itr != _nameMap.end()) {
					// 古いリソースは名前との関連付けを外す
					SHdl h = itr->second;
					base_type::_refSH(h).data.stp = nullptr;
					// 古いハンドルの所有権は外部が持っているのでここでは何もしない

					auto lh = cb();
					itr->second = lh.get();
					base_type::_refSH(lh.get()).data.stp = &(itr->first);
					// 名前ポインタはそのまま流用
					return std::move(lh);
				}
				// エントリの新規作成
				auto lh = cb();
				itr = _nameMap.insert(std::make_pair(std::forward<KEY>(key), lh.get())).first;
				// 名前登録
				auto& ent = base_type::_refSH(lh.get());
				ent.data.stp = &itr->first;
				return std::move(lh);
			}
			template <class KEY, class CB>
			std::pair<LHdl,bool> _acquire(KEY&& key, CB cb) {
				// 既に同じ名前が登録されていたら既存のハンドルを返す
				auto itr = _nameMap.find(key);
				if(itr != _nameMap.end())
					return std::make_pair(LHdl(itr->second), false);

				auto lh = cb();
				itr = _nameMap.emplace(std::forward<KEY>(key), lh.get()).first;
				auto& ent = base_type::_refSH(lh.get());
				ent.data.stp = &(itr->first);
				return std::make_pair(std::move(lh), true);
			}

		public:
			using base_type::acquire;

			template <class KEY, class... Args>
			LHdl replace_emplace(KEY&& key, Args&&... args) {
				auto fn = [&](){ return base_type::acquire(std::forward<Args>(args)...); };
				return _replace(std::forward<KEY>(key), fn);
			}
			//! 同じ要素が存在したら置き換え
			template <class KEY, class DATA>
			LHdl replace(KEY&& key, DATA&& dat) {
				auto fn = [&](){ return base_type::acquire(std::forward<DATA>(dat)); };
				return _replace(std::forward<KEY>(key), fn);
			}
			//! 名前付きリソースの作成
			/*! \return [リソースハンドル,
			 *			新たにエントリが作成されたらtrue, 既存のキーが使われたらfalse] */
			template <class KEY, class DATA>
			std::pair<LHdl,bool> acquire(KEY&& key, DATA&& dat) {
				auto fn = [&]() { return base_type::acquire(std::forward<DATA>(dat)); };
				return _acquire(std::forward<KEY>(key), fn);
			}
			template <class KEY, class... Args>
			std::pair<LHdl,bool> emplace(KEY&& key, Args&&... args) {
				auto fn = [&](){ return base_type::acquire(std::forward<Args>(args)...); };
				return _acquire(std::forward<KEY>(key), fn);
			}

			//! キーを指定してハンドルを取得
			LHdl getFromKey(const std::string& key) const {
				auto itr = _nameMap.find(key);
				if(itr != _nameMap.end())
					return itr->second;
				return LHdl();
			}

			bool release(SHandle sh) override {
				auto& ent = base_type::_refSH(sh);
				const std::string* stp = ent.data.stp;
				if(stp) {
					return base_type::releaseWithCallback(sh, [this, stp](typename base_type::Entry&){
						// 名前も消す
						_nameMap.erase(_nameMap.find(*stp));
					});
				}
				return base_type::release(sh);
			}
	};
}
