#pragma once
#include <cstdint>
#include <string>
#include "abstbuff.hpp"

namespace spn {
	namespace Text {
		// エンコードタイプ
		enum class CODETYPE {
			UTF_8,
			UTF_16LE,
			UTF_16BE,
			UTF_32LE,
			UTF_32BE,
			ANSI
		};
		//! SJISコードか判定する
		bool sjis_isMBChar(char c);
		//! SJISコードにおける文字数判定
		int sjis_strlen(const char* str);
		//! エンコードタイプと判定に使った文字数を返す
		std::pair<CODETYPE, int> GetEncodeType(const void* ptr);
		//! 文字列の先頭4文字から32bitチャンクを生成
		long MakeChunk(const char* str);
		//! "ASCII文字の16進数値"を数値へ
		uint32_t CharToHex(uint32_t ch);
		//! ASCII文字の4桁の16進数値を数値へ
		uint32_t StrToHex4(const char* src);
		uint32_t HexToChar(uint32_t hex);
		//! Base64変換
		int base64(char* dst, std::size_t n_dst, const char* src, int n);
		int base64toNum(char* dst, std::size_t n_dst, const char* src, int n);
		// URL変換
		int url_encode_OAUTH(char* dst, std::size_t n_dst, const char* src, int n);
		int url_encode(char* dst, std::size_t n_dst, const char* src, int n);
		// UTF-16関連
		bool utf16_isSurrogate(char16_t c);
		bool utf16_isSpace(char16_t c);
		bool utf16_isLF(char16_t c);
		bool utf16_isPrivate(char16_t c);	// 私用領域なら1，サロゲート私用領域なら2を返す(予定)
		// UTF16 <-> UTF8 相互変換
		std::u16string UTFConvertTo16(c8Str src);
		std::u16string UTFConvertTo16(c32Str src);
		std::u32string UTFConvertTo32(c16Str src);
		std::u32string UTFConvertTo32(c8Str src);
		std::string UTFConvertTo8(c16Str src);
		std::string UTFConvertTo8(c32Str src);
		void WriteData(void* pDst, char32_t val, int n);

		// nread, nwriteはバイト数ではなく文字数を表す
		// UTF変換(主に内部用)
		Code UTF16To32(char32_t src);
		Code UTF32To16(char32_t src);
		Code UTF8To32(char32_t src);
		//! 不正なシーケンスを検出すると例外を発生させる
		Code UTF8To32_s(char32_t src);
		Code UTF32To8(char32_t src);
		Code UTF8To16(char32_t src);
		Code UTF16To8(char32_t src);
	};
}

