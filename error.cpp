#include "error.hpp"

namespace spn {
	thread_local std::string tls_errMsgTmp;
}
LogOUT g_logOut = [](const std::string& str){
	#ifdef ANDROID
		LOGI(str.c_str());
	#else
		std::cout << str << std::endl;
	#endif
};
// 未使用だとtls_errMsgTmpがリンク時に消されてしまう為
void Dummy_ErrMsg() {
	spn::tls_errMsgTmp.clear();
}
void LogOutput(const std::string& s) {
	g_logOut(s);
}
