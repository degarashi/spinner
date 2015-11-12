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
		const std::string c_testDir = "test_dir";
		Dir MakeTestDir(MTRandom& rd, int nAction, UPIFile* ft) {
			Dir dir(g_apppath.plain_utf32());
			dir <<= c_testDir;
			if(dir.isFile() || dir.isDirectory())
				Dir::RemoveAll(dir.plain_utf8());
			MakeRandomFileTree(dir, rd, nAction, ft);
			return dir;
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
			// 現在パスから見た親フォルダ全てに対してコールバック関数を呼ぶ
			void UpperCallback(const Dir& dir, std::function<void (const Dir&)> cb) {
				Dir tdir(dir);
				while(tdir.segments() > 0) {
					cb(tdir);
					tdir.popBack();
				}
				cb(tdir);
			};
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

			struct Ntf : ActionNotify {
				History&			_hist;
				StringSet			&_nameset,
									&_orig_pathset;
				SPString			_basePath;
				const int			_nseg;
				Ntf(History& hist, const SPString& basePath, int nseg, StringSet& nameset, StringSet& orig_pathset):
					_hist(hist),
					_nameset(nameset),
					_orig_pathset(orig_pathset),
					_basePath(basePath),
					_nseg(nseg)
				{}
				void _addModifyEvent(const Dir& d) {
					_hist.addEvent(new FEv_Modify(_basePath, d.PathBlock::plain_utf8(false), true));
				}
				auto _getAddModF() {
					return [this](auto&& d){ _addModifyEvent(d); };
				}

				void onCreate(const Dir& path) override {
					// ファイル作成イベントを記録
					_hist.addEvent(new FEv_Create(_basePath, path.getSegment_utf8(_nseg, PathBlock::End), false));
					// 上位ディレクトリ(Modify)
					UpperCallback(Dir(path.getSegment_utf32(_nseg, path.segments()-1)), _getAddModF());
				}
				void onModify(const Dir& path, const FStatus& prev, const FStatus& cur) override {
					// 当該ファイル(Access / Write)
					std::string relp = path.getSegment_utf8(_nseg, PathBlock::End);
					if(prev.ftime.tmAccess != cur.ftime.tmAccess)
						_hist.addEvent(new FEv_Access(_basePath, relp, false));
					_hist.addEvent(new FEv_Modify(_basePath, relp, false));
				}
				void onModify_Size(const Dir& path, const FStatus& /*prev*/, const FStatus& /*cur*/) override {
					// 当該ファイル(Write)
					_hist.addEvent(new FEv_Modify(_basePath, path.getSegment_utf8(_nseg, PathBlock::End), false));
				}
				void onDelete(const Dir& path) override {
					// 当該ファイル(Delete)
					_hist.addEvent(new FEv_Remove(_basePath, path.getSegment_utf8(_nseg, PathBlock::End), false));
					// 上位ディレクトリ(Modify)
					UpperCallback(Dir(path.getSegment_utf32(_nseg, path.segments()-1)), _getAddModF());
				}
				void onMove_Pre(const Dir& from, const Dir& to) override {
					std::string pathFrom = from.getSegment_utf8(_nseg, Dir::End),
								pathTo = to.getSegment_utf8(_nseg, Dir::End);
					bool bDir = from.isDirectory();
					// 上位ディレクトリ(From / To : Modify)
					Dir tdir(pathFrom);
					tdir.popBack();
					UpperCallback(tdir, _getAddModF());
					tdir.setPath(pathTo);
					tdir.popBack();
					UpperCallback(tdir, _getAddModF());
					// 当該ファイル (Remove / Create)
					_hist.removeEvent(from);
					if(_orig_pathset.count(from.plain_utf8()) == 1) {
						_hist.removeEvent(from);
						_hist.addEvent(new FEv_Remove(_basePath, pathFrom, bDir));
					}
					// 下層エントリ (Remove)
					LowerCallback(from, [this](const Dir& d) {
						_nameset.emplace(d.getLast_utf8());
						_hist.removeEvent(d);
						if(_orig_pathset.count(d.plain_utf8()) == 1)
							_hist.addEvent(new FEv_Remove(_basePath, d.getSegment_utf8(_nseg, PathBlock::End), d.isDirectory()));
					});

					_hist.addEvent(new FEv_Create(_basePath, pathTo, bDir));
				}
				void onMove_Post(const Dir& /*from*/, const Dir& to) override {
					// 下層エントリ (Create)
					LowerCallback(to, [this](const Dir& d) {
						_nameset.emplace(d.getLast_utf8());
						_hist.addEvent(new FEv_Create(_basePath, d.getSegment_utf8(_nseg, PathBlock::End), d.isDirectory()));
					});
				}
				void onAccess(const Dir& path, const FStatus& prev, const FStatus& cur) override {
					// アクセス前後で時刻が変わっていたらイベントを書き込む
					if(prev.ftime.tmAccess != cur.ftime.tmAccess) {
						// 当該ファイル (Access)
						_hist.addEvent(new FEv_Access(_basePath, path.getSegment_utf8(_nseg, Dir::End), false));
					}
				}
				void onUpDir(const Dir& /*path*/) override {}
				void onDownDir(const Dir& /*path*/) override {}
			};
			auto basePath = std::make_shared<std::string>(dir.plain_utf8());
			Ntf ntf(hist,
					basePath,
					dir.segments(),
					nameset,
					orig_pathset);
			MakeRandomAction(rd, orig_nameset, nameset, dir, 20, ntf);

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
