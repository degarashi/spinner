#include "error.hpp"
LogOUT g_logOut = [](const std::string& str){
	#ifdef ANDROID
		LOGI(str.c_str());
	#else
		std::cout << str << std::endl;
	#endif
};
// 未使用だとtls_errMsgTmpがリンク時に消されてしまう為
void Dummy_ErrMsg() {
}
void LogOutput(const std::string& s) {
	g_logOut(s);
}
void LogOutput(const ::spn::SourcePos& pos, const std::string& msg) {
	boost::format str("at %1% [%2%]:%3%\t%4%");
	ConcatMessage(str, pos.filename, pos.funcname_short, pos.line, msg.c_str());
	LogOutput(str.str());
}

