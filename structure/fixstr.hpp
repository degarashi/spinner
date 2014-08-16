#pragma once
#include <cstring>
#include <cwchar>
#include <assert.h>

namespace spn {
	namespace {
		namespace fixstr_i {
			template <class CT>
			struct FixStrF;
			template <>
			struct FixStrF<char> {
				static void copy(char* dst, int n, const char* src) {
					std::strncpy(dst, src, n);
				}
				static int cmp(const char* v0, const char* v1) {
					return std::strcmp(v0, v1);
				}
			};
			template <>
			struct FixStrF<wchar_t> {
				static void copy(wchar_t* dst, int n, const wchar_t* src) {
					std::wcsncpy(dst, src, n);
				}
				static int cmp(const wchar_t* v0, const wchar_t* v1) {
					return std::wcscmp(v0, v1);
				}
			};
		}
	}
	//! 固定長文字列
	template <class CT, int N>
	struct FixStr {
		typedef fixstr_i::FixStrF<CT> FS;
		typedef CT value_type;
		enum {length=N};
		CT	str[N];

		FixStr(const CT* src=(const CT*)L"") {
			FS::copy(str, N, src);
		}
		FixStr(const CT* src_b, const CT* src_e) {
			int nC = src_e-src_b;
			assert(nC <= sizeof(CT)*(N-1));
			std::memcpy(str, src_b, nC);
			str[nC] = '\0';
		}
		operator const CT* () const {
			return str;
		}
		bool operator < (const CT* s) const {
			return FS::cmp(str, s) < 0;
		}
		bool operator == (const CT* s) const {
			return !FS::cmp(str, s);
		}
		bool operator != (const CT* s) const {
			return !(*this == s);
		}
		FixStr& operator = (const FixStr& s) {
			FS::copy(str, N, s);
			return *this;
		}
		CT& operator [] (int n) {
			return str[n];
		}
		bool operator < (const FixStr& s) const {
			return FS::cmp(str, s) < 0;
		}
		bool operator == (const FixStr& s) const {
			return !FS::cmp(str, s);
		}
		bool operator != (const FixStr& s) const {
			return !(*this == s);
		}
		bool empty() const {
			return str[0] == '\0';
		}
		void clear() {
			str[0] = '\0';
		}
	};
	namespace fixstr_i {}
}

