#include "zip.hpp"
#include <zlib.h>
#include <cstring>

namespace spn {
	namespace zip {
		namespace {
			uint32_t GetSignature(std::istream& is) {
				uint32_t sig;
				is.read(reinterpret_cast<char*>(&sig), sizeof(sig));
				is.seekg(int(-sizeof(sig)), is.cur);
				return sig;
			}
			template <class T>
			void ReadData(T& dst, std::istream& is) {
				is.read(reinterpret_cast<char*>(&dst), sizeof(T)); }
			template <class T>
			void ReadArray(T& dst, size_t len, std::istream& is) {
				dst.resize(len);
				is.read(reinterpret_cast<char*>(&dst[0]), len); }
		}

		UPtr<LocalHeader> LocalHeader::Read(std::istream& is) {
			if(GetSignature(is) != SIGNATURE)
				return nullptr;

			auto* ret = new LocalHeader;
			Core& core = ret->core;
			ReadData(core, is);
			ReadArray(ret->file_name, core.file_namelen, is);
			ReadArray(ret->extra_field, core.extra_fieldlen, is);
			ret->data_pos = is.tellg();
			is.seekg(core.compressed_size, is.cur);
			return UPtr<LocalHeader>(ret);
		}
		bool LocalHeader::Skip(std::istream& is) {
			if(GetSignature(is) != SIGNATURE)
				return false;
			LocalHeader hdr;
			ReadData(hdr.core, is);
			is.seekg(hdr.core.file_namelen + hdr.core.extra_fieldlen + hdr.core.compressed_size, is.cur);
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
					if(_ptr != (&_buff[0] + _buff.size()))
						throw std::runtime_error("something wrong");
				}
			};
			struct OutStream {
				std::ostream& _os;
				constexpr static size_t BUFFSIZE = 4096;
				uint8_t	_tmp[BUFFSIZE];
				size_t	_nWrite;
				size_t _totalWrite;

				OutStream(std::ostream& os, size_t size): _os(os), _nWrite(0), _totalWrite(size) {}
				size_t availOut() const { return BUFFSIZE - _nWrite; }
				void advance(size_t num) {
					_nWrite += num;
					if(_nWrite > BUFFSIZE)
						throw std::runtime_error("something wrong");
					_totalWrite -= num;
					if(_nWrite >= BUFFSIZE/2) {
						_os.write(reinterpret_cast<const char*>(_tmp), _nWrite);
						_nWrite = 0;
					}
				}
				uint8_t* getPtr() { return _tmp + _nWrite; }
				void finalize() const {
					if(_nWrite > 0)
						_os.write(reinterpret_cast<const char*>(_tmp), _nWrite);
					if(_totalWrite != 0)
						throw std::runtime_error("something wrong");
				}
			};
			template <class OB>
			void Decompress(OB& ob, std::istream& is, size_t input_len, size_t output_len) {
				z_stream z;
				z.zalloc = Z_NULL;
				z.zfree = Z_NULL;
				z.opaque = Z_NULL;
				inflateInit(&z);

				constexpr size_t TMP_BUFFSIZE = 4096;
				char tmp[TMP_BUFFSIZE];
				size_t len = std::min(TMP_BUFFSIZE-2, input_len);
				auto* num = reinterpret_cast<uint16_t*>(tmp);
				*num = 0x7880;
				*num = (*num+30) / 31 * 31;
				std::swap(tmp[0],tmp[1]);
				is.read(tmp+2, len);
				input_len -= len;
				len += 2;
				for(;;) {
					z.next_in = reinterpret_cast<uint8_t*>(tmp);
					z.avail_in = len;
					z.next_out = ob.getPtr();
					z.avail_out = ob.availOut();
					size_t nRead = z.avail_in,
							nWrite = z.avail_out;
					int res = inflate(&z, Z_NO_FLUSH);
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
						is.read(tmp + z.avail_in, len);
						input_len -= len;
						len += z.avail_in;
					}
				}
				inflateEnd(&z);
			}
			using SizePair = std::pair<size_t, size_t>;
			SizePair LoadHeader(std::istream& is) {
				LocalHeader hdr;
				auto& core = hdr.core;
				ReadData(core, is);
				if(core.signature != LocalHeader::SIGNATURE)
					throw std::runtime_error("invalid local header");
				is.seekg(core.file_namelen + core.extra_fieldlen, is.cur);
				if(core.compress_method == 0)
					return SizePair(size_t(core.compressed_size), size_t(core.compressed_size));
				return SizePair(size_t(core.compressed_size), size_t(core.uncompressed_size));
			}
		}
		ByteBuff LocalHeader::Extract(std::istream& is) {
			auto sz = LoadHeader(is);
			if(sz.first == sz.second) {
				// 無圧縮
				ByteBuff ret(sz.first);
				is.read(reinterpret_cast<char*>(&ret[0]), sz.first);
				return std::move(ret);
			}
			OutBuffer ob(sz.second);
			Decompress(ob, is, sz.first, sz.second);
			return std::move(ob._buff);
		}
		void LocalHeader::Extract(std::ostream& os, std::istream& is) {
			auto sz = LoadHeader(is);
			if(sz.first == sz.second) {
				// 無圧縮
				constexpr size_t BUFFSIZE = 4096;
				char tmp[BUFFSIZE];
				for(;;) {
					size_t len = std::min(sz.first, BUFFSIZE);
					is.read(tmp, len);
					os.write(tmp, len);
					if(sz.first <= len)
						return;
					sz.first -= len;
				}
			}
			OutStream ofs(os, sz.second);
			Decompress(ofs, is, sz.first, sz.second);
		}

		UPtr<EndHeader> EndHeader::Read(std::istream& is) {
			if(GetSignature(is) != SIGNATURE)
				return nullptr;

			auto* ret = new EndHeader;
			Core& core = ret->core;
			ReadData(core, is);
			ReadArray(ret->comment, core.commentlen, is);
			return UPtr<EndHeader>(ret);
		}
		UPtr<DirHeader> DirHeader::Read(std::istream& is) {
			if(GetSignature(is) != SIGNATURE)
				return nullptr;

			auto* ret = new DirHeader;
			Core& core = ret->core;
			ReadData(core, is);
			ReadArray(ret->file_name, core.file_namelen, is);
			ReadArray(ret->extra_field, core.extra_fieldlen, is);
			ReadArray(ret->comment, core.commentlen, is);
			return UPtr<DirHeader>(ret);
		}

		ZipFile::ZipFile(std::istream& is) {
			for(;;) {
				if(LocalHeader::Skip(is)) {
				} else if(auto up = DirHeader::Read(is)) {
					_hdrDir.emplace_back(std::move(up));
				} else if(auto up = EndHeader::Read(is)) {
					_hdrEnd = std::move(up);
					break;
				} else
					throw std::runtime_error("unknown format");
			}
		}
		const ZipFile::DirHeaderL& ZipFile::headers() const {
			return _hdrDir;
		}
		ByteBuff ZipFile::extract(int n, std::istream& is) const {
			is.seekg(_hdrDir[n]->core.relative_offset, is.beg);
			return LocalHeader::Extract(is);
		}
		void ZipFile::extract(std::ostream& os, int n, std::istream& is) const {
			is.seekg(_hdrDir[n]->core.relative_offset, is.beg);
			LocalHeader::Extract(os, is);
		}
	}
}
