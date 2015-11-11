#include "test.hpp"
#include "dir.hpp"
#include "watch.hpp"
#include "../filetree.hpp"
#include "filetree.hpp"
#include "../random.hpp"
#include <fstream>
#include <unordered_set>
#include <cstdio>

namespace spn {
	namespace test {
		namespace {
			const std::string c_testDir = "test_dir";
			//! テスト用にファイル・ディレクトリ構造を作る
			Dir MakeTestDir(MTRandom& rd, int nAction, UPIFile* ft) {
				Dir dir(g_apppath.plain_utf32());
				dir <<= c_testDir;
				if(dir.isFile() || dir.isDirectory())
					Dir::RemoveAll(dir.plain_utf8());
				MakeRandomFileTree(dir, rd, nAction, ft);
				return dir;
			}
		}
		class FileTreeTest : public RandomTestInitializer {};
		TEST_F(FileTreeTest, ConstructFileTree) {
			auto rd = getRand();
			UPIFile ft0;
			Dir dir = MakeTestDir(rd, rd.getUniform<int>({10, 1000}), &ft0);
			// ファイルシステムからディレクトリ構造ツリーを構築
			auto ft1 = FileTree::ConstructFTree(dir);
			// ツリーを比較
			EXPECT_EQ(*ft0, *ft1);
		}

		namespace {
			//! FEvのハッシュ値を求める
			struct FEv_hash {
				size_t operator()(const FEv* e) const {
					return std::hash<std::string>()(*e->basePath) -
						std::hash<std::string>()(e->name) *
						std::hash<bool>()(e->isDir) +
						std::hash<uint32_t>()(e->getType());
				}
			};
			//! FEvとUPFEvの比較
			struct FEv_equal {
				bool operator()(const UPFEv& fe, const FEv* fp) const {
					return (*this)(fe.get(), fp);
				}
				bool operator()(const FEv* fp, const UPFEv& fe) const {
					return (*this)(fp, fe.get());
				}
				bool operator()(const UPFEv& fe0, const UPFEv& fe1) const {
					return (*this)(fe0.get(), fe1.get());
				}
				bool operator()(const FEv* fp0, const FEv* fp1) const {
					return *fp0 == *fp1;
				}
			};
			void LowerCallback(const Dir& dir, std::function<void (const Dir&)> cb) {
				cb(dir);
				Dir::EnumEntryWildCard(dir.plain_utf8() + "/*", [&cb](const Dir& d) {
					if(d.isDirectory()) {
						LowerCallback(d, cb);
					} else
						cb(d);
				});
			}
		}
		TEST_F(FileTreeTest, Difference) {
			auto rd = getRand();
			Dir dir = MakeTestDir(rd, 5, nullptr);
			// ファイルシステムからディレクトリ構造ツリーを構築
			auto ft0 = FileTree::ConstructFTree(dir);
			// ファイル時間更新の関係で一秒待つ
			std::this_thread::sleep_for(std::chrono::seconds(1));

			// ランダムにファイルを変更
			StringSet orig_nameset,
					orig_pathset,
					  nameset;
			LowerCallback(dir, [&](const Dir& d){
				orig_pathset.emplace(d.plain_utf8());
				orig_nameset.emplace(d.getLast_utf8());
			});

			// 以下のアクションをランダムに起こす
			enum Action {
				Act_Create,		// ファイル作成
				Act_Modify,		// ファイルデータ変更(サイズはそのまま)
				Act_Modify_Size,// ファイルのサイズを変更
				Act_Delete,		// ファイル又はディレクトリ削除
				Act_Move,		// ファイル移動
				Act_Access,		// アクセスタイムを更新
				Act_UpDir,		// ディレクトリを1つ上がる
				Act_DownDir,	// ディレクトリを1つ降りる(現階層にディレクトリがない場合は何もしない)
				NumAction
			};
			using StringV = std::vector<std::string>;

			// ランダムにファイルやディレクトリを選択(一度選んだファイルは二度と選ばない)
			auto fnSelect = [&nameset, &rd](const Dir& dir, uint32_t flag, bool bAdd) {
				StringV ps;
				auto path = dir.plain_utf8() + "/*";
				Dir::EnumEntryWildCard(path, [&nameset, &ps, flag](const Dir& dir) {
					FStatus fs = dir.status();
					if(fs.flag & flag) {
						std::string name = dir.getLast_utf8();
						if(nameset.count(name) == 0)
							ps.push_back(std::move(name));
					}
				});
				if(ps.empty())
					return std::string();
				int idx = rd.getUniform<int>({0, ps.size()-1});
				if(bAdd)
					nameset.emplace(ps[idx]);
				return std::move(ps[idx]);
			};
			auto fnSelectFile = [&fnSelect](const Dir& dir, bool bAdd) {
				return fnSelect(dir, FStatus::FileType, bAdd);
			};
			auto fnSelectDir = [&fnSelect](const Dir& dir, bool bAdd) {
				return fnSelect(dir, FStatus::DirectoryType, bAdd);
			};
			auto fnSelectBoth = [&fnSelect](const Dir& dir, bool bAdd) {
				return fnSelect(dir, FStatus::FileType | FStatus::DirectoryType, bAdd);
			};
			// ファイルツリーのランダムな場所へのパスを作成
			auto fnGenerateRandomPath = [&orig_nameset, &nameset, &fnSelectDir, &rd](const Dir& dir) {
				Dir tdir(dir);
				for(;;) {
					// 階層を降りる or 留まる
					if(rd.getUniform<int>({0, 1}) == 0) {
						auto ps = fnSelectDir(tdir, false);
						if(!ps.empty()) {
							tdir <<= ps;
							continue;
						}
					}
					for(;;) {
						auto name = RandomName(rd, nameset);
						if(orig_nameset.count(name) == 0) {
							tdir <<= name;
							break;
						}
					}
					return tdir.getSegment_utf8(dir.segments(), PathBlock::End);
				}
			};
			// 現在パスから見た親フォルダ全てに対してコールバック関数を呼ぶ
			auto fnUpperCallback = [](const Dir& dir, std::function<void (const Dir&)> cb) {
				Dir tdir(dir);
				while(tdir.segments() > 0) {
					cb(tdir);
					tdir.popBack();
				}
				cb(tdir);
			};

			int nseg_base = dir.segments();
			int layer = 0;
			class History {
				public:
					using FEvS = std::unordered_map<const FEv*, UPFEv, FEv_hash, FEv_equal>;
					FEvS	_hist;
					// [fullpath] -> Event pointer list
					using InfoM = std::unordered_map<std::string, std::vector<const FEv*>>;
					InfoM	_info;
					const StringSet& _orig_pathset;
				public:
					History(const StringSet& s): _orig_pathset(s) {}
					void addEvent(FEv* e) {
						std::string path = *e->basePath + "/" + e->name;
						auto res = _hist.emplace(e, UPFEv(e));
						if(res.second)
							_info[path].emplace_back(e);
						else
							Assert(Trap, _info.count(path)>0)
					}
					//! 関連イベントを全て消す
					/*! \param[in] path フルパス */
					void removeEvent(const Dir& path) {
						// 下層のイベントを先に消す
						if(path.isDirectory()) {
							Dir::EnumEntryWildCard(path.plain_utf8() + "/*", [=](const Dir& d){
								removeEvent(d);
							});
						}
						auto itr = _info.find(path.plain_utf8());
						if(itr != _info.end()) {
							Assert(Trap, !itr->second.empty())
							for(auto* ev : itr->second) {
								auto itr2 = _hist.find(ev);
								Assert(Trap, itr2!=_hist.end())
								_hist.erase(itr2);
							}
							_info.erase(itr);
						}
					}
			} hist(orig_pathset);
			auto basePath = std::make_shared<std::string>(dir.plain_utf8());

			auto fnAddModifyEvent = [&hist, &basePath](const Dir& d) {
				hist.addEvent(new FEv_Modify(basePath, d.PathBlock::plain_utf8(false), true));
			};
			for(int j=0 ; j<20 ; j++) {
				switch(rd.getUniform<int>({0, Action::NumAction})) {
					case Action::Act_Create: {
						std::string str = RandomName(rd, nameset);
						PlaceRandomFile(rd, dir, str);
						dir <<= str;
						// ファイル作成イベントを記録
						hist.addEvent(new FEv_Create(basePath, dir.getSegment_utf8(nseg_base, PathBlock::End), false));
						dir.popBack();
						// 上位ディレクトリ(Modify)
						fnUpperCallback(Dir(dir.getSegment_utf32(nseg_base, PathBlock::End)), fnAddModifyEvent);
						break; }
					case Action::Act_Modify: {
						auto ps = fnSelectFile(dir, true);
						if(!ps.empty()) {
							dir <<= ps;

							auto st = dir.status();
							// 先頭1バイトを書き換え
							{
								char ch;
								FILE* fp = ::fopen(dir.plain_utf8().c_str(), "r+");
								::fread(&ch, 1, 1, fp);
								++ch;
								::fseek(fp, 0, SEEK_SET);
								::fwrite(&ch, 1, 1, fp);
								#ifdef UNIX
									::fsync(::fileno(fp));
								#endif
								::fclose(fp);
							}
							auto st2 = dir.status();


							// 当該ファイル(Access / Write)
							std::string relp = dir.getSegment_utf8(nseg_base, PathBlock::End);
							if(st.ftime.tmAccess != st2.ftime.tmAccess)
								hist.addEvent(new FEv_Access(basePath, relp, false));
							hist.addEvent(new FEv_Modify(basePath, relp, false));
							dir.popBack();
						}
						break; }
					case Action::Act_Modify_Size: {
						auto ps = fnSelectFile(dir, true);
						if(!ps.empty()) {
							dir <<= ps;
							EXPECT_FALSE(dir.isDirectory());
							// 末尾1バイト追加
							{
								char ch = 0xfe;
								FILE* fp = ::fopen(dir.plain_utf8().c_str(), "a");
								::fwrite(&ch, 1, 1, fp);
								#ifdef UNIX
									::fsync(::fileno(fp));
								#endif
								::fclose(fp);
							}

							// 当該ファイル(Write)
							hist.addEvent(new FEv_Modify(basePath, dir.getSegment_utf8(nseg_base, PathBlock::End), false));
							dir.popBack();
						}
						break; }
					case Action::Act_Delete: {
						auto ps = fnSelectFile(dir, true);
						if(!ps.empty()) {
							dir <<= ps;
							EXPECT_FALSE(dir.isDirectory());
							dir.remove();

							// 当該ファイル(Delete)
							hist.addEvent(new FEv_Remove(basePath, dir.getSegment_utf8(nseg_base, PathBlock::End), false));
							dir.popBack();
							// 上位ディレクトリ(Modify)
							fnUpperCallback(Dir(dir.getSegment_utf32(nseg_base, PathBlock::End)), fnAddModifyEvent);
						}
						break; }
					case Action::Act_Move: {
						auto ps = fnSelectBoth(dir, true);
						if(!ps.empty()) {
							dir <<= ps;
							std::string pathFrom = dir.getSegment_utf8(nseg_base, PathBlock::End),
										pathTo = fnGenerateRandomPath(Dir(*basePath));
							EXPECT_NE(pathFrom, pathTo);
							bool bDir = dir.isDirectory();
							// 上位ディレクトリ(From / To : Modify)
							Dir tdir(pathFrom);
							tdir.popBack();
							fnUpperCallback(tdir, fnAddModifyEvent);
							tdir.setPath(pathTo);
							tdir.popBack();
							fnUpperCallback(tdir, fnAddModifyEvent);
							// 当該ファイル (Remove / Create)
							hist.removeEvent(dir);
							if(orig_pathset.count(dir.plain_utf8()) == 1) {
								hist.removeEvent(dir);
								hist.addEvent(new FEv_Remove(basePath, pathFrom, bDir));
							}
							// 下層エントリ (Remove)
							LowerCallback(Dir(*basePath + '/' + pathFrom), [&](const Dir& d) {
								nameset.emplace(d.getLast_utf8());
								hist.removeEvent(d);
								if(orig_pathset.count(d.plain_utf8()) == 1)
									hist.addEvent(new FEv_Remove(basePath, d.getSegment_utf8(nseg_base, PathBlock::End), d.isDirectory()));
							});

							hist.addEvent(new FEv_Create(basePath, pathTo, bDir));
							dir.move(*basePath + '/' + pathTo);
							dir.popBack();

							// 下層エントリ (Create)
							LowerCallback(Dir(*basePath + '/' + pathTo), [&](const Dir& d) {
								nameset.emplace(d.getLast_utf8());
								hist.addEvent(new FEv_Create(basePath, d.getSegment_utf8(nseg_base, PathBlock::End), d.isDirectory()));
							});
						}
						break; }
					case Action::Act_Access: {
						auto ps = fnSelectFile(dir, true);
						if(!ps.empty()) {
							dir <<= ps;
							auto st = dir.status();
							{
								// fsyncを使いたいのでfopenでファイルを開く
								char c;
								FILE* fp = ::fopen(dir.plain_utf8().c_str(), "r");
								::fread(&c, 1, 1, fp);
								#ifdef UNIX
									::fsync(::fileno(fp));
								#endif
								::fclose(fp);
							}
							auto st2 = dir.status();
							// アクセス前後で時刻が変わっていたらイベントを書き込む
							if(st.ftime.tmAccess != st2.ftime.tmAccess) {
								// 当該ファイル (Access)
								hist.addEvent(new FEv_Access(basePath, dir.getSegment_utf8(nseg_base, PathBlock::End), false));
							}
							dir.popBack();
						}
						break; }
					case Action::Act_UpDir: {
						if(layer > 0) {
							dir.popBack();
							--layer;
						}
						break; }
					case Action::Act_DownDir: {
						auto ps = fnSelectDir(dir, false);
						if(!ps.empty()) {
							dir <<= ps;
							++layer;
						}
						break; }
				}
			}
			while(layer-- > 0)
				dir.popBack();
			// 変更が検出できてるかチェック
			UPIFile ft1 = FileTree::ConstructFTree(dir);

			// moveで移動したファイル群は全部使用済みフラグを付ける
			struct MyVisitor : FRecvNotify {
				History& _hist;
				MyVisitor(History& h): _hist(h) {}

				void check(const UPFEv& e) {
					auto itr = _hist._hist.find(e.get());
					EXPECT_NE(itr, _hist._hist.end());
					_hist._hist.erase(itr);
				}

				#define DEF_CHECK(typ)	void event(const typ& e, const SPData& /*ud*/) override { check(UPFEv(new typ(e))); }
				DEF_CHECK(FEv_Create)
				DEF_CHECK(FEv_Access)
				DEF_CHECK(FEv_Modify)
				DEF_CHECK(FEv_Remove)
				DEF_CHECK(FEv_MoveFrom)
				DEF_CHECK(FEv_MoveTo)
				DEF_CHECK(FEv_Attr)
			} visitor(hist);

			// 想定されたイベントを消して行けばちょうどゼロになる筈
			PathBlock pb;
			FileTree::VisitDifference(visitor, basePath, ft0.get(), ft1.get(), pb);
			EXPECT_EQ(hist._hist.size(), size_t(0));
		}
	}
}
