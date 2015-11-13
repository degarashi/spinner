#include "test.hpp"
#include "watch.hpp"
#include "filetree.hpp"
#include "../filetree.hpp"

namespace spn {
	namespace test {
		class FileWatchTest: public RandomTestInitializer {};
		namespace {
			class FEvSet : public std::unordered_map<const FEv*, UPFEv, FEv_hash, FEv_equal> {
				public:
					void addEvent(FEv* e) {
						emplace(e, UPFEv(e));
					}
			};
			class INodeNtf : public FRecvNotify {
				private:
					FEvSet	_set;
				public:
					template <class T>
					void _event(const T& e) {
						_set.addEvent(new T(e));
					}
					void event(const FEv_Create& e, const SPData& /*ud*/) override { _event(e); }
					void event(const FEv_Access& e, const SPData& /*ud*/) override { _event(e); }
					void event(const FEv_Modify& e, const SPData& /*ud*/) override { _event(e); }
					void event(const FEv_Remove& e, const SPData& /*ud*/) override { _event(e); }
					void event(const FEv_MoveFrom& e, const SPData& /*ud*/) override { _event(e); }
					void event(const FEv_MoveTo& e, const SPData& /*ud*/) override { _event(e); }
					void event(const FEv_Attr& e, const SPData& /*ud*/) override { _event(e); }

					const FEvSet& get() const {
						return _set;
					}
					bool has(const FEv* e) const {
						return _set.find(e) != _set.end();
					}
					void clear() {
						_set.clear();
					}
			};
			class Ntf : public ActionNotify {
				private:
					FEvSet			_set;
					const SPString	_basePath;
					const int		_nseg;
				public:
					Ntf(const SPString& basePath, int nseg):
						_basePath(basePath),
						_nseg(nseg)
					{}
					void onCreate(const Dir& path) override {
						bool bDir = path.isDirectory();
						_set.addEvent(new FEv_Create(_basePath, path.getSegment_utf8(_nseg, Dir::End), bDir));
					}
					void onModify(const Dir& path, const FStatus& /*prev*/, const FStatus& /*cur*/) override {
						bool bDir = path.isDirectory();
						_set.addEvent(new FEv_Modify(_basePath, path.getSegment_utf8(_nseg, Dir::End), bDir));
					}
					void onModify_Size(const Dir& path, const FStatus& /*prev*/, const FStatus& /*cur*/) override {
						bool bDir = path.isDirectory();
						_set.addEvent(new FEv_Modify(_basePath, path.getSegment_utf8(_nseg, Dir::End), bDir));
					}
					void onDelete(const Dir& path) override {
						bool bDir = path.isDirectory();
						_set.addEvent(new FEv_Remove(_basePath, path.getSegment_utf8(_nseg, Dir::End), bDir));
					}
					void onMove_Pre(const Dir& from, const Dir& to) override {
						bool bDir = from.isDirectory();
						_set.addEvent(new FEv_MoveFrom(_basePath, from.getSegment_utf8(_nseg, Dir::End), bDir, 0));
						int fseg = from.segments(),
							tseg = to.segments();
						if(fseg == tseg) {
							if(from.getSegment_utf32(Dir::Beg, fseg-1) == to.getSegment_utf32(Dir::Beg, tseg-1))
								_set.addEvent(new FEv_MoveTo(_basePath, to.getSegment_utf8(_nseg, Dir::End), bDir, 0));
						}
					}
					void onAccess(const Dir& path, const FStatus& /*prev*/, const FStatus& /*cur*/) override {
						bool bDir = path.isDirectory();
						_set.addEvent(new FEv_Access(_basePath, path.getSegment_utf8(_nseg, Dir::End), bDir));
					}
					const FEvSet& get() const {
						return _set;
					}
					void clear() {
						_set.clear();
					}
			};
		}
		TEST_F(FileWatchTest, Watch) {
			auto rd = getRand();
			UPIFile ft0;
			Dir dir = MakeTestDir(rd, rd.getUniform<int>({1, 10}), &ft0);

			FNotify notify;
			notify.addWatch(dir.plain_utf8(), ~0);

			Ntf ntf(std::make_shared<std::string>(dir.plain_utf8()), dir.segments());
			INodeNtf inodeNtf;
			FEvSet ntfSet;
			StringSet nameset,
					  orig_nameset;
			auto basePath = dir.plain_utf8();
			for(int i=0 ; i<10 ; ++i) {
				// 何かアクションを起こし、通知と比較
				int n = rd.getUniform<int>({1,10});
				while(--n >= 0)
					MakeRandomActionSingle(rd, orig_nameset, nameset, basePath, dir, ntf);

				std::this_thread::sleep_for(std::chrono::milliseconds(30));
				notify.procEvent(inodeNtf);

				// 少なくともテストで起こしたアクションがWatchクラスで補足されていなければならない
				for(auto& s : ntf.get())
					EXPECT_TRUE(inodeNtf.has(s.first));

				ntf.clear();
				inodeNtf.clear();
				std::this_thread::sleep_for(std::chrono::milliseconds(30));
			}
		}
	}
}
