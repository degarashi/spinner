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
	struct FEv {
		SPString		basePath;
		std::string		name;
		bool			isDir;

		bool operator == (const FEv& f) const;
		FEv(const SPString& base, const std::string& nm, bool bDir);
		virtual FileEvent getType() const = 0;
		virtual const char* getName() const = 0;
		virtual void print(std::ostream& os) const;
	};
	using UPFEv = std::unique_ptr<FEv>;

	struct FRecvNotify;
	class FNotify {
		using UPFEv = std::unique_ptr<FEv>;
		using DSC = typename FNotifyDep::DSC;
		struct Ent {
			DSC			dsc;
			SPString	basePath;
			SPData		udata;
		};

		using PathToEnt = std::map<std::string, Ent>;
		using DSCToEnt = std::map<DSC, Ent*>;
		PathToEnt	_path2ent;
		DSCToEnt	_dsc2ent;

		FNotifyDep	_dep;
		public:
			//! ポーリングで変更イベント取得
			void procEvent(FRecvNotify& ntf);
			void procEvent(std::function<void (const FEv&, const SPData&)> cb);
			//! 監視ポイントを追加
			/*! 既に同じパスが登録されていたら上書き
				\param[in] path 監視対象のパス
				\param[in] mask 通知するイベントの種類フラグ
				\param[in] udata エントリーに関連付けるユーザーデータ */
			void addWatch(const std::string& path, uint32_t mask, const SPData& udata=SPData());
			//! 監視ポイントを削除
			void remWatch(const std::string& path);
	};
	struct FEv_Create : FEv {
		using FEv::FEv;
		using FEv::operator==;
		FileEvent getType() const override;
		const char* getName() const override;
	};
	struct FEv_Access : FEv {
		using FEv::FEv;
		using FEv::operator==;
		FileEvent getType() const override;
		const char* getName() const override;
	};
	struct FEv_Modify : FEv {
		using FEv::FEv;
		using FEv::operator==;
		FileEvent getType() const override;
		const char* getName() const override;
	};
	struct FEv_Remove : FEv {
		using FEv::FEv;
		using FEv::operator==;
		FileEvent getType() const override;
		const char* getName() const override;
	};
	// MoveFromとMoveToはFileTreeの比較で検出出来ない = CreateとRemoveに別れる
	struct FEv_MoveFrom : FEv {
		uint32_t	cookie;

		bool operator == (const FEv& e) const;
		FEv_MoveFrom(const SPString& base, const std::string& name, bool bDir, uint32_t cookieID);
		FileEvent getType() const override;
		const char* getName() const override;
		void print(std::ostream& os) const override;
	};
	struct FEv_MoveTo : FEv {
		uint32_t	cookie;

		bool operator == (const FEv& e) const;
		FEv_MoveTo(const SPString& base, const std::string& name, bool bDir, uint32_t cookieID);
		FileEvent getType() const override;
		const char* getName() const override;
		void print(std::ostream& os) const override;
	};
	struct FEv_Attr : FEv {
		using FEv::FEv;
		using FEv::operator==;
		const char* getName() const override;
		FileEvent getType() const override;
	};
	struct FRecvNotify {
		virtual void event(const FEv_Create& e, const SPData& ud) {}
		virtual void event(const FEv_Access& e, const SPData& ud) {}
		virtual void event(const FEv_Modify& e, const SPData& ud) {}
		virtual void event(const FEv_Remove& e, const SPData& ud) {}
		virtual void event(const FEv_MoveFrom& e, const SPData& ud) {}
		virtual void event(const FEv_MoveTo& e, const SPData& ud) {}
		virtual void event(const FEv_Attr& e, const SPData& ud) {}
	};
}
