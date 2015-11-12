#include "filetree.hpp"
#include "../structure/range.hpp"
#include "../random.hpp"
#include "../dir.hpp"
#include "../filetree.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace spn {
	namespace test {
		namespace {
			const spn::RangeI c_nameLen{3,16},
								c_sizeLen{8, 256},
								c_wordRange{'A', 'Z'};
		}
		std::string RandomName(MTRandom& rd, StringSet& nameset) {
			for(;;) {
				char ftmp[c_nameLen.to+1];
				int fname = rd.getUniform<int>(c_nameLen);
				for(int k=0 ; k<fname ; k++)
					ftmp[k] = rd.getUniform<int>(c_wordRange);
				ftmp[fname] = '\0';

				std::string str(ftmp);
				if(nameset.count(str) == 0) {
					nameset.insert(str);
					return str;
				}
			}
		}
		void PlaceRandomFile(MTRandom& rd, const Dir& d, const std::string& name) {
			Dir dir(d);
			dir <<= name;
			auto path = dir.plain_utf8();
			std::ofstream ofs(path.c_str(), std::ios::binary);
			int fsize = rd.getUniform<int>(c_sizeLen);
			char tmp[c_sizeLen.to];
			for(int k=0 ; k<fsize ; k++)
				tmp[k] = rd.getUniform<int>({0, 0xff});
			ofs.write(tmp, fsize);
		}
		std::string CreateRandomFile(MTRandom& rd, StringSet& nameset, const Dir& dir) {
			std::string name = RandomName(rd, nameset);
			PlaceRandomFile(rd, dir, name);
			return name;
		}
		void MakeRandomFileTree(const Dir& d, MTRandom& rd, int nFile, UPIFile* pDst) {
			Dir dir(d);
			dir.mkdir(FStatus::AllRead);
			std::vector<DirInfo*>	pDi;
			if(pDst) {
				DirInfo* tmp;
				pDst->reset(tmp = new DirInfo(d.getLast_utf8(), DirDep::Status(d.plain_utf8())));
				pDi.emplace_back(tmp);
			}

			// 一度使ったファイル名は使わない
			StringSet nameset;
			int layer = 0;
			for(int j=0 ; j<nFile ; j++) {
				switch(rd.getUniform<int>({0, 2})) {
					case 0:
						// ディレクトリを1つ上がる
						if(layer > 0) {
							FStatus stat = DirDep::Status(dir.plain_utf8());
							dir.popBack();
							if(!pDi.empty()) {
								pDi.back()->fstat = stat;
								pDi.pop_back();
							}
							--layer;
						}
						break;
					case 1: {
						// 下層にディレクトリ作製 & 移動
						bool bRetry = true;
						while(bRetry) {
							try {
								std::string str = RandomName(rd, nameset);
								dir <<= str;
								dir.mkdir(FStatus::AllRead);
								if(!pDi.empty()) {
									DirInfo* tmp;
									pDi.back()->fmap.emplace(str, UPIFile(tmp = new DirInfo(str, FStatus())));
									pDi.emplace_back(tmp);
								}
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
						std::string name = CreateRandomFile(rd, nameset, dir);
						if(!pDi.empty()) {
							dir <<= name;
							FStatus stat = DirDep::Status(dir.plain_utf8());
							dir.popBack();
							pDi.back()->fmap.emplace(name, UPIFile(new FileInfo(name, stat)));
						}
						break; }
				}
			}
			// ディレクトリを1つ上がる
			while(!pDi.empty()) {
				FStatus stat = DirDep::Status(dir.plain_utf8());
				dir.popBack();
				pDi.back()->fstat = stat;
				pDi.pop_back();
			}
		}

		void MakeRandomAction(MTRandom& rd, StringSet& orig_nameset, StringSet& nameset, const Dir& path, int n, ActionNotify& ntf) {
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

			Dir dir(path);
			auto basePath = dir.plain_utf8();
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
			int layer = 0;
			for(int i=0 ; i<n ; i++) {
				switch(rd.getUniform<int>({0, Action::NumAction})) {
					case Action::Act_Create: {
						std::string str = RandomName(rd, nameset);
						PlaceRandomFile(rd, dir, str);
						dir <<= str;
						ntf.onCreate(dir);
						dir.popBack();
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
							ntf.onModify(dir, st, dir.status());
							dir.popBack();
						}
						break; }
					case Action::Act_Modify_Size: {
						auto ps = fnSelectFile(dir, true);
						if(!ps.empty()) {
							dir <<= ps;
							EXPECT_FALSE(dir.isDirectory());
							auto prev = dir.status();
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

							ntf.onModify_Size(dir, prev, dir.status());
							dir.popBack();
						}
						break; }
					case Action::Act_Delete: {
						auto ps = fnSelectFile(dir, true);
						if(!ps.empty()) {
							dir <<= ps;
							EXPECT_FALSE(dir.isDirectory());
							ntf.onDelete(dir);
							dir.remove();
							dir.popBack();
						}
						break; }
					case Action::Act_Move: {
						auto ps = fnSelectBoth(dir, true);
						if(!ps.empty()) {
							dir <<= ps;
							std::string pathTo = fnGenerateRandomPath(Dir(basePath));
							Dir dirTo(basePath + '/' + pathTo);

							EXPECT_NE(dir, dirTo);
							ntf.onMove_Pre(dir, dirTo);
							dir.move(dirTo.plain_utf8());
							ntf.onMove_Post(dir, dirTo);
							dir.popBack();
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
							ntf.onAccess(dir, st, dir.status());
							dir.popBack();
						}
						break; }
					case Action::Act_UpDir: {
						if(layer > 0) {
							dir.popBack();
							--layer;

							ntf.onUpDir(dir);
						}
						break; }
					case Action::Act_DownDir: {
						auto ps = fnSelectDir(dir, false);
						if(!ps.empty()) {
							dir <<= ps;
							++layer;

							ntf.onDownDir(dir);
						}
						break; }
				}
			}
		}
	}
}
