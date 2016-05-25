#include "error.hpp"

namespace spn {
	Log::OutF* Log::s_logOut = nullptr;
	void Log::Initialize() {
		auto f = [](const std::string& str){
			#ifdef ANDROID
				LOGI(str.c_str());
			#else
				std::cout << str << std::endl;
			#endif
		};
		s_logOut = new OutF(f);
	}
	void Log::Terminate() {
		delete s_logOut;
	}
	void Log::Output(const std::string& s) {
		(*s_logOut)(s);
	}
	void Log::SetOutputF(const OutF& f) {
		delete s_logOut;
		s_logOut = new OutF(f);
	}
	Log::OutF* Log::GetOutputF() {
		return s_logOut;
	}
	std::ostream& operator << (std::ostream& s, const SourcePos& p) {
		using std::endl;
		return s
			<< "at file:\t" << p.filename << endl
			<< "at function:\t" << p.funcname << endl
			<< "on line:\t" << p.line << endl;
	}
}
// 未使用だとtls_errMsgTmpがリンク時に消されてしまう為
void Dummy_ErrMsg() {
}
void LogOutput(const ::spn::SourcePos& pos, const std::string& msg) {
	boost::format str("at %1% [%2%]:%3%\t%4%");
	ConcatMessage(str, pos.filename, pos.funcname_short, pos.line, msg.c_str());
	::spn::Log::Output(str.str());
}
