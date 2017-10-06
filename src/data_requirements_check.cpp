#include "allscale/data_requirements_check.hpp"


namespace allscale {
namespace runtime {


// -- an implementation of the check functionality to be utilized if enabled --

void mark_readable(void* from, void* to) {
	// TODO: implement
}

void remove_readable(void* from, void* to) {
	// TODO: implement
}

void check_read(void* base, std::size_t size) {
	// TODO: implement
}

void mark_readwriteable(void* from, void* to) {
//	std::cout << "Granting access to " << from << " - " << to << "\n";
	// TODO: implement
}

void remove_readwriteable(void* from,void* to) {
//	std::cout << "Revoking access to " << from << " - " << to << "\n";
	// TODO: implement
}

void check_write(void* base, std::size_t) {
//	std::cout << "Checking access to " << base << "\n";
	// TODO: implement
}

bool summarize_access_checks_internal() {
	// TODO: implement
	return true; // all fine, for now
}



} // end namespace runtime
} // end namespace allscale

