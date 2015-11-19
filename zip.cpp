#include "zip.hpp"
#include <zlib.h>
#include <cstring>
#include "error.hpp"

namespace spn {
	namespace zip {
		namespace {
			const ErrorMsgList cs_initError[] = {
				{Z_OK, "no error"},
				{Z_MEM_ERROR, "memory error"},
				{Z_STREAM_ERROR, "invalid deflate level"},
				{Z_VERSION_ERROR, "zlib version is not compatible"},
				{0, std::string()}
			},
			cs_procError[] = {
				{Z_OK, "no error"},
				{Z_STREAM_ERROR, "invalid stream state"},
				{Z_BUF_ERROR, "not enough buffer"},
				{0, std::string()}
			},
			cs_endError[] = {
				{Z_OK, "no error"},
				{Z_STREAM_ERROR, "invalid stream state"},
				{Z_DATA_ERROR, "invalid data memory(already freed?)"},
				{0, std::string()}
			};
		};
		ExceptionBase::ExceptionBase(int flag, const std::string& msg, const ErrorMsgList* elst):
			std::runtime_error(""),
			_flag(flag), _origMsg(msg)
		{
			std::string m("unknown error");
			auto* e = elst;
			while(!e->second.empty()) {
				if(e->first == flag) {
					m = e->second;
					break;
				}
				++e;
			}
			m += ": ";
			m += _origMsg;
			reinterpret_cast<std::runtime_error&>(*this) = std::runtime_error(m);
		}
		InitException::InitException(int flag, const std::string& msg): ExceptionBase(flag, msg, cs_initError) {}
		ProcException::ProcException(int flag, const std::string& msg): ExceptionBase(flag, msg, cs_procError) {}
		EndException::EndException(int flag, const std::string& msg): ExceptionBase(flag, msg, cs_endError) {}

		namespace {
			uint32_t GetSignature(AdaptStream& as) {
				uint32_t sig;
				as.read(&sig, sizeof(sig));
				as.seekg(-int32_t(sizeof(sig)), as.cur);
				return sig;
			}
			template <class T>
			void ReadData(T& dst, AdaptStream& as) {
				as.read(reinterpret_cast<char*>(&dst), sizeof(T)); }
			template <class T>
			void ReadArray(T& dst, size_t len, AdaptStream& as) {
				dst.resize(len);
				as.read(reinterpret_cast<char*>(&dst[0]), len); }
		}

		UPtr<LocalHeader> LocalHeader::Read(AdaptStream& as) {
			if(GetSignature(as) != SIGNATURE)
				return nullptr;

			auto* ret = new LocalHeader;
			Core& core = ret->core;
			ReadData(core, as);
			ReadArray(ret->file_name, core.file_namelen, as);
			ReadArray(ret->extra_field, core.extra_fieldlen, as);
			ret->data_pos = as.tellg();
			as.seekg(core.compressed_size, as.cur);
			return UPtr<LocalHeader>(ret);
		}
		bool LocalHeader::Skip(AdaptStream& as) {
			if(GetSignature(as) != SIGNATURE)
				return false;
			LocalHeader hdr;
			ReadData(hdr.core, as);
			as.seekg(hdr.core.file_namelen + hdr.core.extra_fieldlen + hdr.core.compressed_size, as.cur);
			return true;
		}
		namespace {
			struct OutBuffer {
				ByteBuff	_buff;
				uint8_t*	_ptr;

				OutBuffer(size_t size): _buff(size), _ptr(&_buff[0]) {}
				size_t availOut() const { return _buff.size() - (_ptr - &_buff[0]); }
				void advance(size_t num) { _ptr += num; }
				uint8_t* getPtr() { return _ptr; }
				void finalize() const {
					AssertP(Trap, _ptr == (&_buff[0] + _buff.size()), "something wrong")
				}
			};
			struct OutStream {
				AdaptOStream& _aos;
				constexpr static size_t BUFFSIZE = 4096;
				uint8_t	_tmp[BUFFSIZE];
				size_t	_nWrite;
				size_t _totalWrite;

				OutStream(AdaptOStream& aos, size_t size): _aos(aos), _nWrite(0), _totalWrite(size) {}
				size_t availOut() const { return BUFFSIZE - _nWrite; }
				void advance(size_t num) {
					_nWrite += num;
					Assert(Trap, _nWrite <= BUFFSIZE, "something wrong")
					_totalWrite -= num;
					if(_nWrite >= BUFFSIZE/2) {
						_aos.write(_tmp, _nWrite);
						_nWrite = 0;
					}
				}
				uint8_t* getPtr() { return _tmp + _nWrite; }
				void finalize() const {
					if(_nWrite > 0)
						_aos.write(_tmp, _nWrite);
					Assert(Trap, _totalWrite == 0, "something wrong")
				}
			};
			template <class OB>
			void Decompress(OB& ob, AdaptStream& as, size_t input_len, size_t /*output_len*/) {
				z_stream z;
				z.zalloc = Z_NULL;
				z.zfree = Z_NULL;
				z.opaque = Z_NULL;
				int flag = inflateInit(&z);
				AssertTArg(Throw, flag == Z_OK, (InitException)(int)(const char*), flag, z.msg)

				constexpr size_t TMP_BUFFSIZE = 4096;
				char tmp[TMP_BUFFSIZE];
				size_t len = std::min(TMP_BUFFSIZE-2, input_len);
				auto* num = reinterpret_cast<uint16_t*>(tmp);
				*num = 0x7880;
				*num = (*num+30) / 31 * 31;
				std::swap(tmp[0],tmp[1]);
				as.read(tmp+2, len);
				input_len -= len;
				len += 2;
				for(;;) {
					z.next_in = reinterpret_cast<uint8_t*>(tmp);
					z.avail_in = len;
					z.next_out = ob.getPtr();
					z.avail_out = ob.availOut();
					size_t nRead = z.avail_in,
							nWrite = z.avail_out;
					flag = inflate(&z, Z_NO_FLUSH);
					AssertTArg(Throw, flag == Z_OK, (ProcException)(int)(const char*), flag, z.msg)
					nRead -= z.avail_in;
					nWrite -= z.avail_out;

					ob.advance(nWrite);
					if(z.avail_in == 0 && input_len == 0) {
						ob.finalize();
						break;
					}
					if(z.avail_in < TMP_BUFFSIZE) {
						std::memmove(tmp, z.next_in, z.avail_in);
						len = std::min(TMP_BUFFSIZE-z.avail_in, input_len);
						as.read(tmp + z.avail_in, len);
						input_len -= len;
						len += z.avail_in;
					}
				}
				flag = inflateEnd(&z);
				AssertTArg(Throw, flag == Z_OK, (EndException)(int)(const char*), flag, z.msg)
			}
			using SizePair = std::pair<size_t, size_t>;
			SizePair LoadHeader(AdaptStream& as) {
				LocalHeader hdr;
				auto& core = hdr.core;
				ReadData(core, as);
				Assert(Trap, core.signature == LocalHeader::SIGNATURE, "invalid local header")
				as.seekg(core.file_namelen + core.extra_fieldlen, as.cur);
				if(core.compress_method == 0)
					return SizePair(size_t(core.compressed_size), size_t(core.compressed_size));
				return SizePair(size_t(core.compressed_size), size_t(core.uncompressed_size));
			}
		}
		ByteBuff LocalHeader::Extract(AdaptStream& as) {
			auto sz = LoadHeader(as);
			if(sz.first == sz.second) {
				// 無圧縮
				ByteBuff ret(sz.first);
				as.read(reinterpret_cast<char*>(&ret[0]), sz.first);
				return ret;
			}
			OutBuffer ob(sz.second);
			Decompress(ob, as, sz.first, sz.second);
			return std::move(ob._buff);
		}
		void LocalHeader::Extract(AdaptOStream& aos, AdaptStream& as) {
			auto sz = LoadHeader(as);
			if(sz.first == sz.second) {
				// 無圧縮
				constexpr size_t BUFFSIZE = 4096;
				char tmp[BUFFSIZE];
				for(;;) {
					size_t len = std::min(sz.first, BUFFSIZE);
					as.read(tmp, len);
					aos.write(tmp, len);
					if(sz.first <= len)
						return;
					sz.first -= len;
				}
			}
			OutStream ofs(aos, sz.second);
			Decompress(ofs, as, sz.first, sz.second);
		}

		UPtr<EndHeader> EndHeader::Read(AdaptStream& as) {
			if(GetSignature(as) != SIGNATURE)
				return nullptr;

			auto* ret = new EndHeader;
			Core& core = ret->core;
			ReadData(core, as);
			ReadArray(ret->comment, core.commentlen, as);
			return UPtr<EndHeader>(ret);
		}
		UPtr<DirHeader> DirHeader::Read(AdaptStream& as) {
			if(GetSignature(as) != SIGNATURE)
				return nullptr;

			auto* ret = new DirHeader;
			Core& core = ret->core;
			ReadData(core, as);
			ReadArray(ret->file_name, core.file_namelen, as);
			ReadArray(ret->extra_field, core.extra_fieldlen, as);
			ReadArray(ret->comment, core.commentlen, as);
			return UPtr<DirHeader>(ret);
		}

		ZipFile::ZipFile(AdaptStream&& as): ZipFile(as) {}
		ZipFile::ZipFile(AdaptStream& as) {
			for(;;) {
				if(LocalHeader::Skip(as)) {
				} else if(auto up = DirHeader::Read(as)) {
					_hdrDir.emplace_back(std::move(up));
				} else if(auto up = EndHeader::Read(as)) {
					_hdrEnd = std::move(up);
					break;
				} else
					Assert(Trap, false, "unknown format")
			}
		}
		const ZipFile::DirHeaderL& ZipFile::headers() const {
			return _hdrDir;
		}
		ByteBuff ZipFile::extract(int n, AdaptStream& as) const {
			as.seekg(_hdrDir[n]->core.relative_offset, as.beg);
			return LocalHeader::Extract(as);
		}
		void ZipFile::extract(AdaptOStream& aos, int n, AdaptStream& as) const {
			as.seekg(_hdrDir[n]->core.relative_offset, as.beg);
			LocalHeader::Extract(aos, as);
		}
	}
}
