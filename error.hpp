#pragma once
#include <boost/preprocessor.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <sstream>
#include "argholder.hpp"

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

#define _Assert_Base(expr, act, ...)			{if(!(expr)) { act.onError(MakeAssertMsg(#expr, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)); }}
#define _AssertT(act, expr, throwtype)			_Assert_Base(expr, (AAct_##act<BOOST_PP_SEQ_ENUM(throwtype)>()), BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(0,throwtype)))
#define _AssertTArg(act, expr, throwtype, ...)	_Assert_Base(expr, (AAct_##act<BOOST_PP_SEQ_ENUM(throwtype)>(__VA_ARGS__)), BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(0,throwtype)))
// 任意の例外クラスを指定してアサートチェック
// 引数の有無で_AssertT(), _AssertTArg()を呼び分ける
#define AssertT(act, expr, ...)					BOOST_PP_IF(BOOST_PP_EQUAL(1, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), _AssertT, _AssertTArg)(act,expr,__VA_ARGS__)
// デフォルトのエラーメッセージでアサートチェック
#define _Assert(act, expr)						_AssertArg(act, expr, "Assertion failed")
#define _AssertArg(act, expr, ...)				_Assert_Base(expr, (AAct_##act<std::runtime_error>()), __VA_ARGS__)
// Act,Exprに続く引数が無いときは_Assert(), あるなら_AssertArg()を呼ぶ
// 例外形式はstd::runtime_errorとして与えられた引数でエラーメッセージを生成
#define Assert(act, ...)						BOOST_PP_IF(BOOST_PP_EQUAL(1, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), _Assert, _AssertArg)(act, __VA_ARGS__)

#ifdef DEBUG
	#define AssertP(act, ...)						Assert(act, __VA_ARGS__)
	#define AssertTP(act, expr, throwtype, ...)		AssertT(act,expr,throwtype, __VA_ARGS__)
#else
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
template <class... Ts>
std::string MakeAssertMsg(const char* base, const char* filename, const char* funcname, int line, const char* fmt, Ts&&... ts) {
	using std::endl;
	boost::format bf(fmt);
	std::stringstream ss;
	ss << ConcatMessage(bf, std::forward<Ts>(ts)...) << endl
		<< base << endl
		<< "at file:\t" << filename << endl
		<< "at function:\t" << funcname << endl
		<< "on line:\t" << line << endl;
	return ss.str();
}

using LogOUT = std::function<void (const std::string&)>;
extern LogOUT g_logOut;
void LogOutput(const std::string& s);
template <class... Ts>
void LogOutput(const char* fmt, Ts&&... ts) {
	boost::format msg(fmt);
	LogOutput(ConcatMessage(msg, std::forward<Ts>(ts)...));
}

//! エラー時にメッセージ出力だけする
template <class E, class... Ts>
struct AAct_Warn {
	template <class... TA>
	AAct_Warn(TA&&...) {}
	void onError(const std::string& str) {
		LogOutput(str);
	}
};
//! エラー時にメッセージ出力と例外の送出を行う
template <class E, class... Ts>
struct AAct_Throw : spn::ArgHolder<Ts...> {
	using spn::ArgHolder<Ts...>::ArgHolder;
	// 例外クラスがconst std::stringでコンストラクトできる場合はエラーメッセージを渡し、残りの引数を無視
	template <class E2, class... TArg, class Dummy=void*,
		class=typename std::enable_if<
			std::is_constructible<E2, const std::string&>::value
			>::type
		>
	static void __Throw(const std::string& str, TArg... /*args*/) {
		throw E(str); }

	// そうでない場合はエラーメッセージをその場で出力し、引数はユーザーが渡したものを指定
	template <class E2, class... TArg,
		class=typename std::enable_if<
			!std::is_constructible<E2, const std::string&>::value
			>::type
		>
	static void __Throw(const std::string& str, TArg... args) {
		AAct_Warn<E2>().onError(str);
		throw E(args...); }

	template <class T2, class... Ts2>
	static void _Throw(const std::string& str, T2&& t, Ts2&&... ts) {
		AAct_Warn<E>().onError(str);
		throw E(std::forward<T2>(t), std::forward<Ts2>(ts)...); }
	void onError(const std::string& str) {
		this->inorder([&str](Ts... args){
			__Throw<E>(str, args...);
		});
	}
};
//! エラー時にデバッガをブレーク、リリース時には例外の送出を行う
template <class E, class... Ts>
struct AAct_Trap : AAct_Throw<E,Ts...> {
	using base_type = AAct_Throw<E,Ts...>;
	using base_type::base_type;
	void onError(const std::string& str) {
		#ifdef DEBUG
			AAct_Warn<E>().onError(str);
			__builtin_trap();
		#endif
		base_type::onError(str);
	}
};
namespace spn {
	template <class Act, class Chk, class... Ts>
	void CheckError(Act&& act, Chk&& chk, const char* filename, const char* funcname, int line, Ts&&... ts) {
		const char* msg = chk.errorDesc(std::forward<Ts>(ts)...);
		if(msg)
			act.onError(MakeAssertMsg(chk.getAPIName(), filename, funcname, line, msg));
	}
	template <class Act, class Chk>
	struct ErrorChecker {
		Act			_act;
		Chk			_chk;
		int			_line;
		const char	*_filename, *_fname;
		ErrorChecker(Act&& act, Chk&& chk, const char* filename, const char* fname, int line): _act(std::move(act)), _chk(std::move(chk)), _line(line), _filename(filename), _fname(fname) {
			chk.reset();
		}
		~ErrorChecker() {
			CheckError(_act, _chk, _filename, _fname, _line);
		}
	};
	template <class Act, class Chk, class Func, class... TsA>
	auto EChk_base(Act&& act, Chk&& chk, const char* filename, const char* fname, int line, const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
		ErrorChecker<typename std::decay<Act>::type, typename std::decay<Chk>::type> chker(std::forward<Act>(act), std::forward<Chk>(chk), filename, fname, line);
		return func(std::forward<TsA>(ts)...);
	}
	template <class Act, class Chk>
	void EChk_base(Act&& act, Chk&& chk, const char* filename, const char* fname, int line) {
		ErrorChecker<typename std::decay<Act>::type, typename std::decay<Chk>::type> chker(std::forward<Act>(act), std::forward<Chk>(chk), filename, fname, line);
	}
	template <class Func, class... TsA>
	auto EChk_pass(const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
		return func(std::forward<TsA>(ts)...);
	}
	template <class Act, class Chk, class Func, class... TsA>
	auto EChk_baseA1(Act&& act, Chk&& chk, const char* filename, const char* fname, int line, const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
		auto val = func(std::forward<TsA>(ts)...);
		CheckError(std::forward<Act>(act), std::forward<Chk>(chk), filename, fname, line, val);
		return val;
	}
	template <class Act, class Chk, class RES>
	RES EChk_baseA2(Act&& act, Chk&& chk, const char* filename, const char* fname, int line, const RES& res) {
		CheckError(std::forward<Act>(act), std::forward<Chk>(chk), filename, fname, line, res);
		return res;
	}
}
