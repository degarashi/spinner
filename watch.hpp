#pragma once
#if defined(UNIX)
	#include "watch_depLinux.hpp"
#elif defined(WIN32)
	#include "watch_depWin.hpp"
#else
	#error "unknown OS"
#endif
#include <vector>
#include <functional>
#include <memory>

namespace spn {
	using UPVoid = std::unique_ptr<void*>;
	struct FEv {
		const std::string&	basePath;
		void*				udata;
		std::string			name;
		bool				isDir;

		FEv(const std::string& base, void* ud, const std::string& nm, bool bDir): basePath(base), udata(ud), name(nm), isDir(bDir) {}
		virtual FileEvent getType() const = 0;
	};
	using UPFEv = std::unique_ptr<FEv>;

	struct UserData {
		virtual ~UserData() {}
	};
	using UData = std::unique_ptr<UserData>;

	struct FRecvNotify;
	class FNotify {
		using UPFEv = std::unique_ptr<FEv>;
		using DSC = typename FNotifyDep::DSC;
		struct Ent {
			DSC			dsc;
			std::string	basePath;
			UData		udata;
		};

		using PathToEnt = std::map<std::string, Ent>;
		using DSCToEnt = std::map<DSC, Ent*>;
		PathToEnt	_path2ent;
		DSCToEnt	_dsc2ent;

		FNotifyDep	_dep;
		public:
			//! ブロッキングの変更イベント取得
			void procEvent(FRecvNotify& ntf);
			//! スレッド作ってそこでprocEvent
			/*! 非ブロッキング関数を使う時には最初に一度だけ呼び出す */
			void beginASync();
			//! 非ブロッキングバージョン
			void procEventASync(FRecvNotify& ntf);

			//! 監視ポイントを追加
			/*! 既に同じパスが登録されていたら上書き
				\param[in] path 監視対象のパス
				\param[in] mask 通知するイベントの種類フラグ
				\param[in] udata エントリーに関連付けるユーザーデータ */
			void addWatch(const std::string& path, uint32_t mask, UData udata=nullptr);
			//! 監視ポイントを削除
			void remWatch(const std::string& path);
	};
	struct FEv_Create : FEv {
		using FEv::FEv;
		FileEvent getType() const override { return FE_Create; }
	};
	struct FEv_Modify : FEv {
		using FEv::FEv;
		FileEvent getType() const override { return FE_Modify; }
	};
	struct FEv_Remove : FEv {
		using FEv::FEv;
		FileEvent getType() const override { return FE_Remove; }
	};
	struct FEv_MoveFrom : FEv {
		uint32_t	cookie;
		FEv_MoveFrom(const std::string& base, void* ud, const std::string& name, bool bDir, uint32_t cookieID):
			FEv(base, ud, name, bDir), cookie(cookieID) {}
		FileEvent getType() const override { return FE_MoveFrom; }
	};
	struct FEv_MoveTo : FEv {
		uint32_t	cookie;
		FEv_MoveTo(const std::string& base, void* ud, const std::string& name, bool bDir, uint32_t cookieID):
			FEv(base, ud, name, bDir), cookie(cookieID) {}
		FileEvent getType() const override { return FE_MoveTo; }
	};
	struct FRecvNotify {
		virtual void event(const FEv_Create& e) {}
		virtual void event(const FEv_Modify& e) {}
		virtual void event(const FEv_Remove& e) {}
		virtual void event(const FEv_MoveFrom& e) {}
		virtual void event(const FEv_MoveTo& e) {}
	};
}
