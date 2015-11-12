#pragma once
#include <string>
#include <unordered_set>
#include <memory>

namespace spn {
	class MTRandom;
	class Dir;
	struct IFTFile;
	struct FStatus;
	using UPIFile = std::unique_ptr<IFTFile>;
	struct FEv;
	using UPFEv = std::unique_ptr<FEv>;
	namespace test {
		extern const std::string c_testDir;
		using StringSet = std::unordered_set<std::string>;
		//! ランダムな以前の結果と重複しない名前を生成
		std::string RandomName(MTRandom& rd, StringSet& nameset);
		//! 指定されたファイル名でランダムデータを書き込む
		void PlaceRandomFile(MTRandom& rd, const Dir& d, const std::string& name);
		//! ランダムな名前、データでファイルを作成する
		std::string CreateRandomFile(MTRandom& rd, StringSet& nameset, const Dir& dir);
		void MakeRandomFileTree(const Dir& d, MTRandom& rd, int nFile, UPIFile* pDst);
		//! テスト用にファイル・ディレクトリ構造を作る
		Dir MakeTestDir(MTRandom& rd, int nAction, UPIFile* ft);

		//! FEvのハッシュ値を求める
		struct FEv_hash {
			size_t operator()(const UPFEv& e) const;
			size_t operator()(const FEv* e) const;
		};
		//! FEvとUPFEvの比較
		struct FEv_equal {
			bool operator()(const UPFEv& fe, const FEv* fp) const;
			bool operator()(const FEv* fp, const UPFEv& fe) const;
			bool operator()(const UPFEv& fe0, const UPFEv& fe1) const;
			bool operator()(const FEv* fp0, const FEv* fp1) const;
		};
		struct ActionNotify {
			virtual void onCreate(const Dir& /*path*/) {}
			virtual void onModify(const Dir& /*path*/, const FStatus& /*prev*/, const FStatus& /*cur*/) {}
			virtual void onModify_Size(const Dir& /*path*/, const FStatus& /*prev*/, const FStatus& /*cur*/) {}
			virtual void onDelete(const Dir& /*path*/) {}
			virtual void onMove_Pre(const Dir& /*from*/, const Dir& /*to*/) {}
			virtual void onMove_Post(const Dir& /*from*/, const Dir& /*to*/) {}
			virtual void onAccess(const Dir& /*path*/, const FStatus& /*prev*/, const FStatus& /*cur*/) {}
			virtual void onUpDir(const Dir& /*path*/) {}
			virtual void onDownDir(const Dir& /*path*/) {}
		};
		//! テスト用ディレクトリ構造に対してランダムなファイル操作を行う(ディレクトリ移動を含まない操作一回分)
		void MakeRandomActionSingle(MTRandom& rd, StringSet& orig_nameset, StringSet& nameset, const std::string& basePath, const Dir& path, ActionNotify& ntf);
		//! テスト用ディレクトリ構造に対してランダムなファイル操作を行う
		void MakeRandomAction(MTRandom& rd, StringSet& orig_nameset, StringSet& nameset, const Dir& path, int n, ActionNotify& ntf);
	}
}
