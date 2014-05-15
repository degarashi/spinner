//! ファイルツリー構造を保持するクラス。とりあえずWindows専用
#pragma once
#include "misc.hpp"
#include <boost/regex.hpp>
#include <unordered_map>
#include "dir.hpp"

namespace spn {
	struct IFTFile;
	using UPIFile = std::unique_ptr<IFTFile>;
	struct FRecvNotify;
	using SPString = std::shared_ptr<std::string>;

	//! ファイルツリーの構築
	struct FileTree {
		enum Type {
			File = 0x01,
			Directory = 0x02
		};
		enum Diff {
			Created = 0x01,
			Deleted = 0x02,
			Accessed = 0x04,
			Writed = 0x08
		};

		static UPIFile ConstructFTree(Dir& dir);
		static UPIFile ConstructFTree(To32Str path);
		static void For_Each(const IFTFile* ft, std::function<void(const std::string&, const IFTFile*)> cb, PathBlock& pblock);
		//! 下層のエントリ全てに対して削除イベントを送出
		static void ThrowRemoveEvent(FRecvNotify& visitor, const SPString& base, const IFTFile* ft, PathBlock& pblock);
		//! 下層のエントリ全てに対し作成イベントを送出
		static void ThrowCreateEvent(FRecvNotify& visitor, const SPString& base, const IFTFile* ft, PathBlock& pblock);
		//! ファイルツリー差分を巡ってコールバック関数を呼ぶ
		static void VisitDifference(FRecvNotify& visitor, const SPString& base, const IFTFile* f0, const IFTFile* f1, PathBlock& pblock);
	};

	struct IFTFile {
		std::string	fname;
		FStatus		fstat;

		virtual bool isDirectory() const = 0;
		virtual FileTree::Type getType() const = 0;

		IFTFile(const std::string& name, const FStatus& fs): fname(name), fstat(fs) {}
		const std::string& getName() const { return fname; }
		const FStatus& getStatus() const { return fstat; }
		virtual bool operator == (const IFTFile& f) const {
			return getType() == f.getType() &&
					fname == f.fname &&
					fstat == f.fstat;
		}
		bool operator != (const IFTFile& f) const {
			return !(*this == f); }

		//! デバッグ用プリント関数
		virtual void print(std::ostream& os, int indent=0) const = 0;
		virtual ~IFTFile() {}
	};
	//! ファイル1つに相当
	struct FileInfo : IFTFile {
		using IFTFile::IFTFile;
		FileTree::Type getType() const override;
		bool isDirectory() const override;
		void print(std::ostream& os, int indent) const override;
	};
	//! ディレクトリに該当
	struct DirInfo : IFTFile {
		using FileMap = std::unordered_map<std::string, UPIFile>;
		FileMap		fmap;

		using IFTFile::IFTFile;
		FileTree::Type getType() const override;
		bool isDirectory() const override;
		bool operator == (const IFTFile& f) const override;
		void print(std::ostream& os, int indent) const override;
	};
}
