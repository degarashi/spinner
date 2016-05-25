#pragma once
#include <boost/preprocessor.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <sstream>
#include "argholder.hpp"
#include "type.hpp"

#ifdef ANDROID
	#include <android/log.h>
	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "threaded_app", __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "threaded_app", __VA_ARGS__))
	/* For debug builds, always enable the debug traces in this library */
	#ifndef NDEBUG
	#  define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_VERBOSE, "threaded_app", __VA_ARGS__))
	#else
	#  define LOGV(...)  ((void)0)
	#endif
#endif

// Debug=assert, Release=throw		[Assert_Trap]
// Debug=assert, Release=none		[Assert_TrapP]
// Debug=throw, Release=throw		[Assert_Throw]
// Debug=throw, Release=none		[Assert_ThrowP]
// Debug=warning, Release=warning	[Assert_Warn]
// Debug=warning, Release=none		[Assert_WarnP]

#define Assert_MakeMessage(expr, ...)				::spn::MakeAssertMsg(#expr, SOURCEPOS, __VA_ARGS__).c_str()
// 任意の例外クラスを指定してアサートチェック
#define AssertT(act, expr, throwtype, ...)			{if(!(expr)) { AAct_##act<throwtype, const char*>(Assert_MakeMessage(expr, __VA_ARGS__)).onError(Assert_MakeMessage(expr, __VA_ARGS__)); }}
#define AssertTArg(act, expr, throwtype, ...)		{if(!(expr)) { AAct_##act<BOOST_PP_SEQ_ENUM(throwtype)>(__VA_ARGS__).onError(Assert_MakeMessage(expr, BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(0,throwtype))), std::true_type()); }}
// デフォルトのエラーメッセージでアサートチェック
#define _Assert(act, expr)						_AssertArg(act, expr, "Assertion failed")
#define _AssertArg(act, expr, ...)				AssertT(act, expr, std::runtime_error, __VA_ARGS__)
// Act,Exprに続く引数が無いときは_Assert(), あるなら_AssertArg()を呼ぶ
// 例外形式はstd::runtime_errorとして与えられた引数でエラーメッセージを生成
#define Assert(act, ...)						BOOST_PP_IF(BOOST_PP_EQUAL(1, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), _Assert, _AssertArg)(act, __VA_ARGS__)
// 必ず失敗するアサート文 (到達してはいけない場所に処理が来たなど)
#define AssertF(act, ...)						{ Assert(act, false, __VA_ARGS__) throw 0; }
#define AssertFT(act, throwtype, ...)			{ AssertT(act, false, throwtype, __VA_ARGS__) throw 0; }

#ifdef DEBUG
	#define AssertFP(act, ...)						AssertF(act, __VA_ARGS__)
	#define AssertFTP(act, throwtype, ...)			AssertFT(act, throwtype, __VA_ARGS__)
	#define AssertP(act, ...)						Assert(act, __VA_ARGS__)
	#define AssertTP(act, expr, throwtype, ...)		AssertT(act,expr,throwtype, __VA_ARGS__)
#else
	#define AssertFP(act, ...)
	#define AssertFTP(act, throwtype, ...)
	#define AssertP(act, ...)
	#define AssertTP(act, expr, throwtype, ...)
#endif
template <class... Ts>
std::string ConcatMessage(boost::format& fmt, Ts&&... /*t*/) { return fmt.str(); }
template <class T, class... Ts>
std::string ConcatMessage(boost::format& fmt, T&& t, Ts&&... ts) {
	fmt % std::forward<T>(t);
	return ConcatMessage(fmt, std::forward<Ts>(ts)...);
}
namespace spn {
	//! ソースコード上の位置を表す情報
	struct SourcePos {
		const char	*filename,
					*funcname,
					*funcname_short;
		int			line;
	};
	template <class... Ts>
	std::string MakeAssertMsg(const char* base, const SourcePos& s, const char* fmt, const Ts&... ts) {
		using std::endl;
		boost::format bf(fmt);
		std::stringstream ss;
		ss << ConcatMessage(bf, ts...) << endl
			<< base << endl
			<< s;
		return ss.str();
	}
	std::ostream& operator << (std::ostream& s, const SourcePos& p);
}

#include "structure/niftycounter.hpp"
namespace spn {
	struct Log {
		using OutF = std::function<void (const std::string&)>;
		static OutF* s_logOut;

		static void Initialize();
		static void Terminate();
		static void Output(const std::string& s);
		template <class... Ts>
		static void Output(const char* fmt, Ts&&... ts) {
			boost::format msg(fmt);
			Output(ConcatMessage(msg, std::forward<Ts>(ts)...));
		}
		static OutF* GetOutputF();
		static void SetOutputF(const OutF& f);
	};
	DEF_NIFTY_INITIALIZER(Log)
}

//! エラー時にメッセージ出力だけする
template <class E, class... Ts>
struct AAct_Warn {
	template <class... TA>
	AAct_Warn(TA&&...) {}
	void onError(const std::string& str, ...) {
		::spn::Log::Output(str);
	}
};
//! エラー時にメッセージ出力と例外の送出を行う
template <class E, class... Ts>
struct AAct_Throw : spn::ArgHolder<Ts...> {
	using spn::ArgHolder<Ts...>::ArgHolder;
	void onError(const std::string& str, std::false_type={}) {
		this->inorder([&str](Ts... args){
			throw E(args...);
		});
	}
	void onError(const std::string& str, std::true_type) {
		#ifdef DEBUG
			AAct_Warn<E>().onError(str);
		#endif
		onError(str, std::false_type());
	}
};
//! エラー時にデバッガをブレーク、リリース時には例外の送出を行う
template <class E, class... Ts>
struct AAct_Trap : AAct_Throw<E,Ts...> {
	using base_type = AAct_Throw<E,Ts...>;
	using base_type::base_type;
	void onError(const std::string& str, ...) {
		#ifdef DEBUG
			AAct_Warn<E>().onError(str);
			__builtin_trap();
		#endif
		base_type::onError(str);
	}
};

#define SOURCEPOS ::spn::SourcePos{__FILE__, __PRETTY_FUNCTION__, __func__, __LINE__}
namespace spn {
	template <class Act, class Chk, class... Ts>
	void EChk(Act&& act, Chk&& chk, const SourcePos& pos, Ts&&... ts) {
		const char* msg = chk.errorDesc(std::forward<Ts>(ts)...);
		if(msg)
			act.onError(MakeAssertMsg(chk.getAPIName(), pos, msg));
	}
	namespace detail {
		template <class RT, class Act, class Chk, class Func, class... TsA>
		RT EChk_base(std::false_type, Act&& act, Chk&& chk, const SourcePos& pos, const Func& func, TsA&&... ts) {
			chk.reset();
			auto res = func(std::forward<TsA>(ts)...);
			EChk(std::forward<Act>(act), std::forward<Chk>(chk), pos);
			return res;
		}
		template <class RT, class Act, class Chk, class Func, class... TsA>
		void EChk_base(std::true_type, Act&& act, Chk&& chk, const SourcePos& pos, const Func& func, TsA&&... ts) {
			chk.reset();
			func(std::forward<TsA>(ts)...);
			EChk(std::forward<Act>(act), std::forward<Chk>(chk), pos);
		}

		template <class T, class Func>
		struct NoneT_cnv {
			using type = T;
		};
		template <class Func>
		struct NoneT_cnv<spn::none_t, Func> {
			using type = ResultOf_t<Func>;
		};
	}

	//! エラーチェック & アサート(APIがエラーを記憶しているタイプ)
	template <class RT, class Act, class Chk, class Func, class... Ts>
	auto EChk_memory(Act&& act, Chk&& chk, const SourcePos& pos, const Func& func, Ts&&... ts) {
		// 戻り値がvoidかそれ以外かで分岐
		using rt_t = typename detail::NoneT_cnv<RT, Func>::type;
		return detail::EChk_base<rt_t>(typename std::is_same<void, rt_t>::type(),
									std::forward<Act>(act),
									std::forward<Chk>(chk),
									pos, func, std::forward<Ts>(ts)...);
	}
	//! エラーチェック & アサート(APIがエラーコードを返すタイプ)
	template <class Act, class Chk, class Func, class... TsA>
	auto EChk_code(Act&& act, Chk&& chk, const SourcePos& pos, const Func& func, TsA&&... ts) {
		auto code = func(std::forward<TsA>(ts)...);
		EChk(std::forward<Act>(act),
					std::forward<Chk>(chk),
					pos,
					code);
		return code;
	}
	//! エラーチェック & アサート(予め取得した結果コードを入力)
	template <class Act, class Chk, class CODE>
	CODE EChk_usercode(Act&& act, Chk&& chk, const SourcePos& pos, const CODE& code) {
		EChk(std::forward<Act>(act),
					std::forward<Chk>(chk),
					pos, code);
		return code;
	}
	#define EChk_memory_a		EChk_memory
	#define EChk_code_a			EChk_code
	#define EChk_usercode_a		EChk_usercode
	#define EChk_a				EChk
	#ifdef DEBUG
		#define EChk_memory_d		EChk_memory
		#define EChk_code_d			EChk_code
		#define EChk_usercode_d		EChk_usercode
		#define EChk_d				EChk
	#else
		#define EChk_d(...)
		// ----- エラーチェック無し(非デバッグモード時) -----
		template <class Act, class Chk, class Func, class... Ts>
		auto EChk_memory_d(Act&&, Chk&&, const SourcePos&, const Func& func, Ts&&... ts) {
			return func(std::forward<Ts>(ts)...);
		}
		template <class... Ts>
		auto EChk_code_d(Ts&&... ts) {
			return EChk_memory_d(std::forward<Ts>(ts)...);
		}
		template <class Act, class Chk, class CODE>
		CODE EChk_usercode_d(Act&&, Chk&&, const SourcePos&, const CODE& code) { return code; }
	#endif
}

// ---- ログ出力 ----
#define PrintLog			LogOutput(SOURCEPOS, "")
#define PrintLogMsg(...)	LogOutput(SOURCEPOS, __VA_ARGS__)
void LogOutput(const ::spn::SourcePos& pos, const std::string& msg);
template <class... Ts>
void LogOutput(const ::spn::SourcePos& pos, const std::string& msg, Ts&&... ts) {
	boost::format str(msg);
	ConcatMessage(str, std::forward<Ts>(ts)...);
	LogOutput(pos, str.str());
}
