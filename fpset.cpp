#include "fpset.hpp"
#include <cstdint>

namespace spn {
	// 1-sign : 5-exp : 10-fract
	Fp16::Fp16() {}
	Fp16::Fp16(float v) {
		setFloat(v);
	}
	Fp16::Fp16(Fp32 fp) {
		int32_t t_exp = int32_t(fp.exp) - 127 + 15;
		// 負数ならビットが全部1になり、それ以外は全部0
		int32_t mask = (t_exp >> 31);

		// 指数のマイナス飽和
		t_exp &= ~mask;
		// 指数のプラス飽和
		int32_t mask2 = ZeroOrFull(t_exp & 0xffffffe0);
		t_exp |= mask2;
		t_exp &= 0x1f;
		// 指数がどちらかに飽和したら仮数部は0とする
		mask |= mask2;
		int32_t t_fract = fp.fract >> 13;
		t_fract &= ~(mask|mask2);

		exp = t_exp;
		sign = fp.sign;
		fract = t_fract;
	}
	float Fp16::asFloat() const {
		Fp32 ret(*this);
		return *reinterpret_cast<float*>(&ret);
	}
	void Fp16::setFloat(float v) {
		Fp32 fp32(v);
		*this = Fp16(fp32);
	}

	Fp32::Fp32() {}
	Fp32::Fp32(float v) {
		setFloat(v);
	}
	Fp32::Fp32(Fp16 fp) {
		int32_t t_fract = int32_t(fp.fract) << 13;
		int32_t t_exp = int32_t(fp.exp);
		int32_t mask = ZeroOrFull(t_exp);			// 指数が0の時に0, それ以外は全部1
		int32_t mask2 = ZeroOrFull(t_exp ^ 0x1f);	// 指数が15の時に0
		int32_t mask3 = ~(mask|mask2);				// 指数が上限または下限の時に0
		t_fract &= mask3;
		t_exp = t_exp - 15 + 127;
		t_exp &= mask;								// 指数のマイナス飽和
		t_exp |= ~mask2 & 0xff;						// 指数のプラス飽和

		exp = t_exp;
		fract = t_fract;
		sign = fp.sign;
	}
	float Fp32::asFloat() const {
		return *reinterpret_cast<const float*>(this);
	}
	void Fp32::setFloat(float v) {
		*this = *reinterpret_cast<Fp32*>(&v);
	}
}
