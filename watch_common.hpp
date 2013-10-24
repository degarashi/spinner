#pragma once
#include <string>
#include <memory>
#include <map>

namespace spn {
	// ファイル構造検知
	enum FileEvent {
		FE_Invalid = 0x00,
		FE_Create = 0x01,
		FE_Modify = 0x02,
		FE_Remove = 0x04,
		FE_MoveFrom = 0x08,
		FE_MoveTo = 0x10,
		FE_Attribute = 0x20
	};
	struct FEv;
	using Callback = std::function<void (FEv&)>;
}
