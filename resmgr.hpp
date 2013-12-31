#pragma once
#include "misc.hpp"
#include "bits.hpp"
#include "macro.hpp"
#include "noseq.hpp"
#include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>
#include "serialization/unordered_map.hpp"
#include "serialization/traits.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/binary_object.hpp>

namespace spn {
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
		private:
			Value _value;

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int ver) {
				ar & _value.value();
			}
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
			explicit operator bool () const;
			bool operator == (const SHandle& sh) const;
			bool operator != (const SHandle& sh) const;
			bool operator < (const SHandle& sh) const;
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

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int ver) {
				ar & _value.value();
			}
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
			explicit operator bool () const;
			bool operator == (const WHandle& wh) const;
			bool operator != (const WHandle& wh) const;
			bool operator < (const WHandle& wh) const;
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

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int ver) {
				ar & _hdl;
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
			//! 解放せずにハンドルを無効化する
			void setNull() {
				_hdl = HDL();
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
			HdlLock& operator = (HDL hdl) {
				reset(hdl);
				return *this;
			}

			operator bool () const { return valid(); }
			operator HDL () const { return _hdl; }

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
		using DownConv = std::is_convertible<DATA, mgr_data>;
		using UpConv = std::is_convertible<mgr_data, DATA>;

		// static_assertion: DATAからMGR::data_typeは異なっていても良いが、アップコンバートかダウンコンバート出来なければならない
		constexpr static int StaticAssertion[(DownConv::value || UpConv::value) ? 0 : -1] = {};

		friend MGR;
		friend class HdlLock<SHandleT>;
		private:
			// データ型をダウンコンバートする場合はMGRからしか許可しない
			template <class DAT,
						class = typename std::enable_if<std::is_convertible<DATA, DAT>::value>::type,
						class = typename std::enable_if<!std::is_same<DAT, DATA>::value>::type>
			SHandleT(const SHandleT<MGR,DAT>& sh): SHandle(sh) {}

		public:
			using WHdl = WHandleT<MGR, DATA>;
			using mgr_type = MGR;
			using data_type = DATA;
			using SHandle::SHandle;
			SHandleT() = default;
			SHandleT(const SHandle& hdl): SHandle(hdl) {}
			// data_typeが異なっていてもアップコンバート可能ならば暗黙的な変換を許可する
			template <class DAT,
					class = typename std::enable_if<std::is_convertible<DAT, DATA>::value>::type>
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
		using DownConv = std::is_convertible<DATA, mgr_data>;
		using UpConv = std::is_convertible<mgr_data, DATA>;

		// static_assertion: DATAからMGR::data_typeは異なっていても良いが、アップコンバートかダウンコンバート出来なければならない
		constexpr static int StaticAssertion[(DownConv::value || UpConv::value) ? 0 : -1] = {};

		friend MGR;
		private:
			// データ型をダウンコンバートする場合はMGRからしか許可しない
			template <class DAT,
						class = typename std::enable_if<std::is_convertible<DATA, DAT>::value>::type,
						class = typename std::enable_if<!std::is_same<DAT, DATA>::value>::type>
			WHandleT(const WHandleT<MGR,DAT>& wh): WHandle(wh) {}
		public:
			using SHdl = SHandleT<MGR, DATA>;
			using mgr_type = MGR;
			using data_type = DATA;
			using WHandle::WHandle;
			WHandleT() = default;
			WHandleT(const WHandle& wh): WHandle(wh) {}
			// data_typeが異なっていてもアップコンバート可能ならば暗黙的な変換を許可する
			template <class DAT,
					class = typename std::enable_if<std::is_convertible<DAT, DATA>::value>::type>
			WHandleT(const WHandleT<MGR,DAT>& wh): WHandle(wh) {}

			//! リソース参照
			/*!	参照が無効なら例外を投げる
			*	\return リソースの参照 */
			HdlLock<SHdl> lock() const { return MGR::_ref().lockLH(*this); }
			bool isHandleValid() const { return MGR::_ref().isHandleValid(*this); }
	};

	//! 型を限定しないリソースマネージャ基底
	class ResMgrBase {
		using RMList = noseq_list<ResMgrBase*, std::allocator, int>;
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
			ar & value & bStp;
			if(!bStp)
				stp = nullptr;
		}
	};
	//! ResWrap<>を取り除く
	template <class T>
	struct DecayWrap {
		using result = T;
	};
	template <class T, class K>
	struct DecayWrap<ResWrap<T,K>> {
		using result = T;
	};
	//! 名前なしリソース (anonymous-only)
	template <class DAT, class DERIVED, template <class> class Allocator=std::allocator>
	class ResMgrA : public Singleton<DERIVED>, public ResMgrBase {
		public:
			using data_type = typename DecayWrap<DAT>::result;
			using ThisType = ResMgrA<DAT,DERIVED,Allocator>;
			using SHdl = SHandleT<ThisType>;
			using WHdl = WHandleT<ThisType>;
			using LHdl = HdlLock<SHdl>;
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
					ar & data & count & accessCount & w_magic;
					#ifdef DEBUG
						ar & magic;
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
				AssertP(Trap, !_bSerializing)
				_bSerializing = true;

				// 最初にMergeフラグを読む
				bool bMerge;
				ar & bMerge;
				normal_serialize(ar, ver);
				if(bMerge) {
					size_t nData;
					ar & nData;

					// 統合ロード(既に同じリソースが存在している場合(アクセスカウンタ一致)は読み込まない)
					std::vector<bool> chk(nData);
					for(size_t i=0 ; i<nData ; i++) {
						uint32_t ac_count;
						uint64_t wmagic;
						ar & ac_count & wmagic;

						bool flag = false;
						if(_dataVec.has(i)) {
							Entry& ent = _dataVec.get(i);
							if(ent.accessCount == ac_count) {
								if(ent.w_magic == wmagic)
									flag = true;
							}
						}
						chk[i] = flag;
					}
					_dataVec._load_partial(ar, chk);
				}
				// 全消しして上書き or IDのみ (フラグで判断)
				ar & _dataVec;

				// シリアライズフラグの解除は自動
				_bSerializing = false;
			}
			template <class Archive>
			void save(Archive& ar, const unsigned int ver) const {
				AssertP(Trap, !_bSerializing)
				_bSerializing = true;

				ar & s_bMerge;
				auto* ths = const_cast<ThisType*>(this);
				ths->normal_serialize(ar, ver);
				if(s_bMerge) {
					s_bMerge = false;
					// 統合ロードの為にリソースハンドル毎に切り出して保存
					size_t nData = _dataVec.size();
					ar & nData;

					std::stringstream ss;
					for(auto& e : _dataVec) {
						// アクセスカウントとWeakマジックナンバーの出力
						ar & e.accessCount & e.w_magic;
					}
					for(size_t i=0 ; i<nData ; i++) {
						// データ本体の出力
						// 一旦ローカルに出力してから・・
						boost::archive::binary_oarchive oa(ss, boost::archive::no_header);
						oa & _dataVec.getValueOP(i);
						// 改めて外部に出力
						auto tmp = ss.str();
						size_t len = tmp.length();
						ar & len & boost::serialization::make_binary_object(&tmp[0], len);

						ss.str("");
						ss.clear();
						ss << std::dec;
					}
					// DataVecのIDSだけ出力
					ar & _dataVec.asIDSType();
					s_bMerge = true;
				} else {
					// 普通に出力
					ar & _dataVec;
				}

				// シリアライズフラグの解除はマニュアル
			}
			template <class Archive>
			void normal_serialize(Archive& ar, const unsigned int ver) {
				#ifdef DEBUG
					ar & _sMagicIndex;
				#endif
				ar & _wMagicIndex & _resID;
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
				return LHdl(sh);
			}
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
			//! 名前なしリソースの作成
			template <class DAT2>
			LHdl acquire(DAT2&& dat) {
				return _acquire(std::forward<DAT2>(dat));
			}
			template <class... Ts>
			LHdl emplace(Ts&&... args) {
				return _acquire(DAT(std::forward<Ts>(args)...));
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
			template <class NDATA>
			using AnotherLHandle = HdlLock<AnotherSHandle<NDATA>>;
			size_t size() const {
				return _dataVec.size();
			}
			//! 主にデバッグ用途
			AVec getNSeq() {
				return std::move(_dataVec);
			}

		protected:
			//! 継承先クラスにて内部データ型をダウンキャストする際に使用
			template <class NDATA, template<class,class> class Handle, class DATA>
			static Handle<ThisType,NDATA> Cast(Handle<ThisType, DATA>&& h) {
				// Handleのprivateなコンストラクタを経由して変換
				return Handle<ThisType,NDATA>(std::move(h)); }
			template <class NDATA, template<class,class> class Handle, class DATA>
			static Handle<ThisType,NDATA> Cast(const Handle<ThisType, DATA>& h) {
				return Handle<ThisType,NDATA>(h); }

			template <class NDATA, template<class> class HL, class DATA>
			static HL<AnotherSHandle<NDATA>> Cast(HL<AnotherSHandle<DATA>>&& lh) {
				return HL<AnotherSHandle<NDATA>>(std::move(lh)); }
			template <class NDATA, template<class> class HL, class DATA>
			static HL<AnotherSHandle<NDATA>> Cast(const HL<AnotherSHandle<DATA>>& lh) {
				return HL<AnotherSHandle<NDATA>>(lh); }
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
			ar & static_cast<T&>(*this);
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
		private:
			using NameMap = std::unordered_map<KEY, typename base_type::SHdl>;
			NameMap		_nameMap;

			template <class KEY2, class CB>
			LHdl _replace(KEY2&& key, CB cb) {
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
				itr = _nameMap.insert(std::make_pair(std::forward<KEY2>(key), lh.get())).first;
				// 名前登録
				auto& ent = base_type::_refSH(lh.get());
				ent.data.stp = &itr->first;
				return std::move(lh);
			}
			template <class KEY2, class CB>
			std::pair<LHdl,bool> _acquire(KEY2&& key, CB cb) {
				KEY key2(std::forward<KEY2>(key));
				// 既に同じ名前が登録されていたら既存のハンドルを返す
				auto itr = _nameMap.find(key2);
				if(itr != _nameMap.end())
					return std::make_pair(LHdl(itr->second), false);

				auto lh = cb();
				itr = _nameMap.emplace(std::move(key2), lh.get()).first;
				auto& ent = base_type::_refSH(lh.get());
				ent.data.stp = &(itr->first);
				return std::make_pair(std::move(lh), true);
			}

			BOOST_SERIALIZATION_SPLIT_MEMBER();
			friend boost::serialization::access;
			template <class Archive>
			void load(Archive& ar, const unsigned int ver) {
				ar & _nameMap & boost::serialization::base_object<base_type>(*this);
				// ポインタの振りなおし
				for(auto& p : _nameMap) {
					auto& data = base_type::_refSH(p.second).data;
					data.stp = &p.first;
				}
			}
			template <class Archive>
			void save(Archive& ar, const unsigned int ver) const {
				ar & _nameMap & boost::serialization::base_object<base_type>(*this);
			}

		public:
			using base_type::acquire;

			template <class KEY2, class... Args>
			LHdl replace_emplace(KEY2&& key, Args&&... args) {
				// 本当は以下の様に書きたいけどgccじゃコンパイルが通らないので・・
				// auto fn = std::bind(base_type::acquire, this, std::forward<Args>(args)...);
				auto fn = std::bind(&base_type::template acquire<Args&&...>, std::ref(*this), RRef(std::forward<Args>(args))...);
				return _replace(std::forward<KEY2>(key), fn);
			}
			//! 同じ要素が存在したら置き換え
			template <class KEY2, class DATA>
			LHdl replace(KEY2&& key, DATA&& dat) {
				// 本当は以下の様に書きたいけどgccじゃコンパイルが通らないので・・
				// auto fn = [&](){ return base_type::acquire(std::forward<DATA>(dat)); };
				auto fn = std::bind(&base_type::template acquire<DATA&&>, std::ref(*this), RRef(std::forward<DATA>(dat)));
				return _replace(std::forward<KEY2>(key), fn);
			}
			//! 名前付きリソースの作成
			/*! \return [リソースハンドル,
			 *			新たにエントリが作成されたらtrue, 既存のキーが使われたらfalse] */
			template <class KEY2, class DATA>
			std::pair<LHdl,bool> acquire(KEY2&& key, DATA&& dat) {
				// 本当は以下の様に書きたいけどgccじゃコンパイルが通らないので・・
				// auto fn = [&]() { return base_type::acquire(std::forward<DATA>(dat)); };
				auto fn = std::bind(&base_type::template acquire<DATA&&>, std::ref(*this), RRef(std::forward<DATA>(dat)));
				return _acquire(std::forward<KEY2>(key), fn);
			}
			template <class KEY2, class... Args>
			std::pair<LHdl,bool> emplace(KEY2&& key, Args&&... args) {
				auto fn = std::bind(&base_type::template emplace<Args&&...>, std::ref(*this), RRef(std::forward<Args>(args))...);
				// 本当は以下の様に書きたいけどgccじゃコンパイルが通らないので・・
 				// auto fn = [&]() { return base_type::emplace(std::forward<Args>(args)...); };
				return _acquire(std::forward<KEY2>(key), fn);
			}

			//! キーを指定してハンドルを取得
			LHdl getFromKey(const KEY& key) const {
				auto itr = _nameMap.find(key);
				if(itr != _nameMap.end())
					return itr->second;
				return LHdl();
			}

			bool release(SHandle sh) override {
				auto& ent = base_type::_refSH(sh);
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
	//! 互換ハンドル一式を定義するマクロ
	#define DEF_HANDLE(mgr, suffix, sp) \
		using HL##suffix = mgr::AnotherLHandle<sp>; \
		using H##suffix = mgr::AnotherSHandle<sp>; \
		using W##suffix = mgr::AnotherWHandle<sp>;
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
	template <class H>
	struct hash<spn::HdlLock<H>> {
		size_t operator()(const spn::HdlLock<H>& h) const {
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
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class), spn::HdlLock, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class), spn::HdlLock, track_never)

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
