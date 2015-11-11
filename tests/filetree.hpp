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
	namespace test {
		using StringSet = std::unordered_set<std::string>;
		//! ランダムな以前の結果と重複しない名前を生成
		std::string RandomName(MTRandom& rd, StringSet& nameset);
		//! 指定されたファイル名でランダムデータを書き込む
		void PlaceRandomFile(MTRandom& rd, const Dir& d, const std::string& name);
		//! ランダムな名前、データでファイルを作成する
		std::string CreateRandomFile(MTRandom& rd, StringSet& nameset, const Dir& dir);
		void MakeRandomFileTree(const Dir& d, MTRandom& rd, int nFile, UPIFile* pDst);
	}
}
