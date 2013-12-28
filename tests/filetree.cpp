#include "test.hpp"
#include "filetree.hpp"
#include <fstream>
#include "../filetree.hpp"
#include "watch.hpp"
#include <unordered_set>
#include <cstdio>

namespace spn {
	namespace test {
		struct FEv_hash {
			size_t operator()(const UPFEv& e) const {
				return std::hash<std::string>()(*e->basePath) -
					std::hash<std::string>()(e->name) *
					std::hash<bool>()(e->isDir) +
					std::hash<uint32_t>()(e->getType());
			}
		};
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
			dir.enumEntryWildCard("*", [&cb](const Dir& d) {
				if(d.isDirectory()) {
					LowerCallback(d, cb);
				} else
					cb(d);
			});
		}
		constexpr int NTEST = 16,
					NITER = 256,
					MAX_NAME = 16,
					MIN_SIZE = 8,
					MAX_SIZE = 256;
		void FileTree_test(ToPathStr initialPath) {
			// テスト用にファイル・ディレクトリ構造を作る
			Dir dir(initialPath.getStringPtr());
			dir <<= "test_dir";
			dir.mkdir(FStatus::AllRead);

			Random& rd = Random::getInstance();
			// 一度使ったファイル名は使わない
			std::unordered_set<std::string> nameset;
			// ランダムな以前の結果と重複しない名前を生成
			auto randomName = [&rd, &nameset]() {
				for(;;) {
					char ftmp[MAX_NAME+1];
					int fname = rd.randomI(3, MAX_NAME);
					for(int k=0 ; k<fname ; k++)
						ftmp[k] = rd.randomI('A', 'Z');
					ftmp[fname] = '\0';

					std::string str(ftmp);
					if(nameset.count(str) == 0) {
						nameset.insert(str);
						return std::move(str);
					}
				}
			};
			// 指定されたファイル名でランダムデータを書き込む
			auto placeRandomFile = [&rd](Dir& dir, const std::string& name) {
				dir <<= name;
				std::ofstream ofs(dir.plain_utf8().c_str(), std::ios::binary);
				int fsize = rd.randomI(MIN_SIZE, MAX_SIZE);
				char tmp[MAX_SIZE];
				for(int k=0 ; k<fsize ; k++)
					tmp[k] = rd.randomIPositive(0xff);
				ofs.write(tmp, fsize);
				dir.popBack();
			};
			// ランダムな名前、データでファイルを作成する
			auto fnCreateRandomFile = [&placeRandomFile, &randomName](Dir& dir) {
				std::string name = randomName();
				placeRandomFile(dir, name);
				return std::move(name);
			};

			int layer = 0;
			for(int j=0 ; j<10 ; j++) {
				switch(rd.randomI(0, 2)) {
					case 0:
						// ディレクトリを1つ上がる
						if(layer > 0) {
							dir.popBack();
							--layer;
						}
						break;
					case 1: {
						// 下層にディレクトリ作製 & 移動
						bool bRetry = true;
						while(bRetry) {
							try {
								std::string str = randomName();
								dir <<= str;
								dir.mkdir(FStatus::AllRead);
								bRetry = false;
							} catch(const std::string& s) {
								// 既に同名のファイルがあったら再トライ
								//TODO: 他のエラーと区別がつかないのを修正
								dir.popBack();
							}
						}
						++layer;
						break; }
					case 2: {
						// カレントにファイルを作成 (ファイル名と内容はランダム)
						fnCreateRandomFile(dir);
						break; }
				}
			}

			SPString basePath(new std::string(To8Str(initialPath.getStringPtr()).moveTo() + "/test_dir"));
			// ファイルツリーを作成
			UPIFile ft0 = FileTree::ConstructFTree(*basePath);

			nameset.clear();
			// ランダムにファイルを変更
			dir.setPath(initialPath.getStringPtr());
			dir <<= "test_dir";

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
			using Path_Size = std::pair<std::string, uint64_t>;
			using Path_SizeL = std::vector<Path_Size>;

			// ランダムにファイルやディレクトリを選択(一度選んだファイルは二度と選ばない)
			auto fnSelect = [&nameset, &rd](const Dir& dir, uint32_t flag) {
				Path_SizeL ps;
				dir.enumEntryWildCard("*", [&nameset, &ps, flag](const Dir& dir) {
					FStatus fs = dir.status();
					if(fs.flag & flag) {
						std::string name = dir.getLast_utf8();
						if(nameset.count(name) == 0)
							ps.push_back(std::make_pair(std::move(name), fs.size));
					}
				});
				if(ps.empty())
					return Path_Size();
				int idx = rd.randomIPositive(ps.size()-1);
				return std::move(ps[idx]);
			};
			auto fnSelectFile = [&fnSelect](const Dir& dir) {
				return fnSelect(dir, FStatus::FileType);
			};
			auto fnSelectDir = [&fnSelect](const Dir& dir) {
				return fnSelect(dir, FStatus::DirectoryType);
			};
			auto fnSelectBoth = [&fnSelect](const Dir& dir) {
				return fnSelect(dir, FStatus::FileType | FStatus::DirectoryType);
			};
			// ファイルツリーのランダムな場所へのパスを作成
			auto fnGenerateRandomPath = [&randomName, &fnSelectDir, &rd](const Dir& dir) {
				Dir tdir(dir);
				for(;;) {
					// 階層を降りる or 留まる
					if(rd.randomI(0, 1) == 0) {
						auto ps = fnSelectDir(tdir);
						if(!ps.first.empty()) {
							tdir <<= ps.first;
							continue;
						}
					}
					tdir <<= randomName();
					return tdir.getSegment_utf8(dir.segments(), PathBlock::End);
				}
			};
			// 現在パスから見た親フォルダ全てに対してコールバック関数を呼ぶ
			auto fnUpperCallback = [](const Dir& dir, int n, std::function<void (const Dir&)> cb) {
				Dir tdir(dir);
				while(n >= 0) {
					cb(tdir);
					--n;
				}
			};

			int nseg_base = Dir(*basePath).segments();
			layer = 0;
			using FEvS = std::unordered_set<UPFEv, FEv_hash, FEv_equal>;
			FEvS hist;

			auto fnAddModifyEvent = [&hist, &basePath](const Dir& d) {
				hist.emplace(new FEv_Modify(basePath, d.plain_utf8(), true));
			};
			for(int j=0 ; j<100 ; j++) {
				switch(rd.randomIPositive(Action::NumAction)) {
					case Action::Act_Create: {
						std::string str = randomName();
						placeRandomFile(dir, str);
						dir <<= str;
						hist.emplace(new FEv_Create(basePath, dir.getSegment_utf8(nseg_base, PathBlock::End), false));
						nameset.insert(std::move(str));
						dir.popBack();
						// 上位ディレクトリ(Modify)
						// TODO: 現在のUnix時間を取得してそれよりアクセス時間が新しければイベント通知
						fnUpperCallback(Dir(dir.getSegment_utf32(nseg_base, PathBlock::End)), layer, fnAddModifyEvent);
						break; }
					case Action::Act_Modify: {
						Path_Size ps = fnSelectFile(dir);
						if(!ps.first.empty()) {
							dir <<= ps.first;

							auto st = dir.status();
							// 先頭1バイトを書き換え
							{
								char ch;
								FILE* fp = ::fopen(dir.plain_utf8().c_str(), "r+");
								::fread(&ch, 1, 1, fp);
								++ch;
								::fseek(fp, 0, SEEK_SET);
								::fwrite(&ch, 1, 1, fp);
								::fsync(::fileno(fp));
								::fclose(fp);
							}
							auto st2 = dir.status();


							// 当該ファイル(Access / Write)
							std::string relp = dir.getSegment_utf8(nseg_base, PathBlock::End);
							if(st.ftime.tmAccess != st2.ftime.tmAccess)
								hist.emplace(new FEv_Access(basePath, relp, false));
							hist.emplace(new FEv_Modify(basePath, relp, false));
							dir.popBack();
						}
						break; }
					case Action::Act_Modify_Size: {
						Path_Size ps = fnSelectFile(dir);
						if(!ps.first.empty()) {
							dir <<= ps.first;
							Assert(Trap, !dir.isDirectory())
							// 末尾1バイト追加
							{
								char ch = 0xfe;
								FILE* fp = ::fopen(dir.plain_utf8().c_str(), "a");
								::fwrite(&ch, 1, 1, fp);
								::fsync(::fileno(fp));
								::fclose(fp);
							}

							// 当該ファイル(Write)
							hist.emplace(new FEv_Modify(basePath, dir.getSegment_utf8(nseg_base, PathBlock::End), false));
							dir.popBack();
						}
						break; }
					case Action::Act_Delete: {
						Path_Size ps = fnSelectFile(dir);
						if(!ps.first.empty()) {
							dir <<= ps.first;
							Assert(Trap, !dir.isDirectory())
							dir.remove();

							// 当該ファイル(Delete)
							hist.emplace(new FEv_Remove(basePath, dir.getSegment_utf8(nseg_base, PathBlock::End), false));
							dir.popBack();
							// 上位ディレクトリ(Modify)
							fnUpperCallback(Dir(dir.getSegment_utf32(nseg_base, PathBlock::End)), layer, fnAddModifyEvent);
						}
						break; }
					case Action::Act_Move: {
						Path_Size ps = fnSelectBoth(dir);
						if(!ps.first.empty()) {
							dir <<= ps.first;
							std::string pathFrom = dir.getSegment_utf8(nseg_base, PathBlock::End),
										pathTo = fnGenerateRandomPath(Dir(*basePath));
							size_t len = std::min(pathFrom.length(), pathTo.length());
							if(std::strncmp(pathFrom.c_str(), pathTo.c_str(), len) == 0)
								continue;
							bool bDir =dir.isDirectory();
							// 当該ファイル (Delete / Create)
							hist.emplace(new FEv_Create(basePath, pathTo, bDir));
							hist.emplace(new FEv_Remove(basePath, pathFrom, bDir));
							// 上位ディレクトリ(From / To : Modify)
							Dir tdir(pathFrom);
							tdir.popBack();
							fnUpperCallback(tdir, std::max(0,layer-1), fnAddModifyEvent);
							tdir.setPath(pathTo);
							tdir.popBack();
							fnUpperCallback(tdir, std::max(0,layer-1), fnAddModifyEvent);
							// 下層エントリ (Delete / Create)
							LowerCallback(Dir(*basePath + '/' + pathFrom), [&nameset, nseg_base, &basePath, &hist](const Dir& d) {
								nameset.emplace(d.getLast_utf8());
								hist.emplace(new FEv_Remove(basePath, d.getSegment_utf8(nseg_base, PathBlock::End), d.isDirectory()));
							});
							dir.move(*basePath + '/' + pathTo);
							dir.popBack();
							LowerCallback(Dir(*basePath + '/' + pathTo), [&nameset, nseg_base, &basePath, &hist](const Dir& d) {
								nameset.emplace(d.getLast_utf8());
								hist.emplace(new FEv_Create(basePath, d.getSegment_utf8(nseg_base, PathBlock::End), d.isDirectory()));
							});
						}
						break; }
					case Action::Act_Access: {
						Path_Size ps = fnSelectFile(dir);
						if(!ps.first.empty()) {
							dir <<= ps.first;
							auto st = dir.status();
							{
								// fsyncを使いたいのでfopenでファイルを開く
								auto len = st.size;
								char c;
								FILE* fp = ::fopen(dir.plain_utf8().c_str(), "r");
								::fread(&c, 1, 1, fp);
								::fsync(::fileno(fp));
								::fclose(fp);
							}
							auto st2 = dir.status();
							// アクセス前後で時刻が変わっていたらイベントを書き込む
							if(st.ftime.tmAccess != st2.ftime.tmAccess) {
								// 当該ファイル (Access)
								hist.emplace(new FEv_Access(basePath, dir.getSegment_utf8(nseg_base, PathBlock::End), false));
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
						Path_Size ps = fnSelectDir(dir);
						if(!ps.first.empty()) {
							dir <<= ps.first;
							++layer;
						}
						break; }
				}
			}
			// 変更が検出できてるかチェック
			UPIFile ft1 = FileTree::ConstructFTree(*basePath);

			ft0->print(std::cout);
			ft1->print(std::cout);

			// moveで移動したファイル群は全部使用済みフラグを付ける

			struct MyVisitor : FRecvNotify {
				std::unordered_set<UPFEv, FEv_hash, FEv_equal>& _hist;
				MyVisitor(std::unordered_set<UPFEv, FEv_hash, FEv_equal>& h): _hist(h) {}

				void check(const UPFEv& e) {
					auto itr = _hist.find(e);
					Assert(Trap, itr!=_hist.end())
					_hist.erase(itr);
				}

				#define DEF_CHECK(typ)	void event(const typ& e, const SPData& ud) override { check(UPFEv(new typ(e))); }
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
			for(auto& e : hist)
				e->print(std::cout);
		}
	}
}
