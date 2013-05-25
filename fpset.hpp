//! classes of floating-point 16bit

namespace spn {
	//! 引数が0なら0を、それ以外なら~0を返す
	template <class T>
	inline T ZeroOrFull(const T& t) {
		return (t | -t) >> (sizeof(T)*8-1);
	}
	
	struct Fp32;
	//! 16bitの浮動小数点数
	/*! GPUに16bitFPを渡す時用
		何か計算する時は一旦floatに戻す */
	struct Fp16 {
		unsigned short	fract : 10,
						exp : 5,
						sign : 1;
		Fp16();
		Fp16(float v);
		Fp16(Fp32 fp);
		
		float asFloat() const;
		void setFloat(float v);
	};
	
	//! floatのラッパークラス
	struct Fp32 {
		unsigned int 	fract : 23,
						exp : 8,
						sign : 1;
		
		Fp32();
		Fp32(float v);
		Fp32(Fp16 fp);
		
		float asFloat() const;
		void setFloat(float v);
	};
}
