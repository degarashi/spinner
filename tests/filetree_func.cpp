#include "filetree.hpp"
#include "../structure/range.hpp"
#include "../random.hpp"
#include "../dir.hpp"
#include "../filetree.hpp"
#include <fstream>

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
	}
}
