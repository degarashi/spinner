#pragma once
#include <stdexcept>

namespace spn {
	class ValidationFail : public std::domain_error {
		using std::domain_error::domain_error;
	};
	class ValidationFailRuntime : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};
}
