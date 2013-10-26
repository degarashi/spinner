#pragma once
#include <memory>
#include <fstream>
#include <vector>

namespace spn {
	template <class T>
	using UPtr = std::unique_ptr<T>;
	using ByteBuff = std::vector<uint8_t>;

	namespace zip {
		using SPos = typename std::istream::streampos;
		struct LocalHeader {
			constexpr static uint32_t SIGNATURE = 0x04034b50;
			struct Core {
				uint32_t	signature;
				uint16_t	version,
							bit_flag,
							compress_method,
							last_modtime,
							last_moddate;
				uint32_t	crc32,
							compressed_size,
							uncompressed_size;
				uint16_t	file_namelen,
							extra_fieldlen;
			} __attribute__ ((packed));

			Core		core;
			std::string	file_name;
			ByteBuff	extra_field;
			SPos		data_pos;

			static UPtr<LocalHeader> Read(std::istream& is);
			static bool Skip(std::istream& is);
			static ByteBuff Extract(std::istream& is);
			static void Extract(std::ostream& os, std::istream& is);
		};

		struct DataDesc {
			constexpr static uint32_t SIGNATURE = 0x08074b50;
			uint32_t	signature,
						crc32,
						compressed_size,
						uncompressed_size;

			static UPtr<DataDesc> Read(std::istream& is);
		};

		struct DirHeader {
			constexpr static uint32_t SIGNATURE = 0x02014b50;
			struct Core {
				uint32_t	signature;
				uint16_t	version,
							needed_version,
							bit_flag,
							compress_method,
							last_modtime,
							last_moddate;
				uint32_t	crc32,
							compressed_size,
							uncompressed_size;
				uint16_t	file_namelen,
							extra_fieldlen,
							commentlen,
							disknumber,
							internal_fileattr;
				uint32_t	external_fileattr;
				int32_t		relative_offset;
			} __attribute__ ((packed));
			Core		core;
			std::string	file_name,
						comment;
			ByteBuff	extra_field;

			static UPtr<DirHeader> Read(std::istream& is);
		};

		struct EndHeader {
			constexpr static uint32_t SIGNATURE = 0x06054b50;
			struct Core {
				uint32_t	signature;
				uint16_t	number_of_disk,
							disk_starts,
							number_of_records,
							total_records;
				uint32_t	size_of_central_directory,
							offset_of_start_of_central;
				uint16_t	commentlen;
			} __attribute__ ((packed));
			Core		core;
			std::string	comment;

			static UPtr<EndHeader> Read(std::istream& is);
		};

		class ZipFile {
			using DirHeaderL = std::vector<UPtr<DirHeader>>;
			DirHeaderL		_hdrDir;
			UPtr<EndHeader>	_hdrEnd;

			public:
				ZipFile(std::istream& is);
				const DirHeaderL& headers() const;
				ByteBuff extract(int n, std::istream& is) const;
				void extract(std::ostream& os, int n, std::istream& is) const;
		};
	}
}
