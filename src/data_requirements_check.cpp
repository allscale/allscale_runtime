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
	// TODO: implement
}

void remove_readwriteable(void* from,void* to) {
	// TODO: implement
}

void check_write(void*, std::size_t) {
	// TODO: implement
}

bool summarize_access_checks_internal() {
	// TODO: implement
	return true; // all fine, for now
}



} // end namespace runtime
} // end namespace allscale

