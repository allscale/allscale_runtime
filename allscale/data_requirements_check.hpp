#ifndef ALLSCALE_DATA_REQUIREMENTS_CHECK_HPP
#define ALLSCALE_DATA_REQUIREMENTS_CHECK_HPP

#include <iostream>

namespace allscale {
namespace runtime {

#ifdef ALLSCALE_RUNTIME_WITH_DATA_REQUIREMENT_CHECKS

// -- an implementation of the check functionality to be utilized if enabled --

inline void mark_readable(void* from, void* to) {
	// TODO: register region
}

inline void remove_readable(void*,void*) {
	// TODO: remove region
}

template<typename T>
const T& check_read(const T& data) {
	// TODO: check address of data element
	return data;
}

inline void mark_readwriteable(void* from, void* to) {
	// TODO: register region
}

inline void remove_readwriteable(void* from,void* to) {
	// TODO: remove region
}

template<typename T>
T& check_write(T& data) {
	// TODO: check address of data element
	return data;
}


#else

// -- empty implementations of the same interface --

inline void mark_readable(void*, void*) {
	// nothing to do
}

inline void remove_readable(void*,void*) {
	// nothing to do
}

template<typename T>
const T& check_read(const T& data) {
	// nothing to check
	return data;
}

inline void mark_readwriteable(void*, void*) {
	// nothing to do
}

inline void remove_readwriteable(void*,void*) {
	// nothing to do
}

template<typename T>
T& check_write(T& data) {
	// nothing to check
	return data;
}

#endif

// -- general utility functionality --

template<typename DataItem>
void mark_readable(const typename DataItem::fragment_type& item, const typename DataItem::region_type& region) {
	// TODO: implement
}

template<typename DataItem>
void remove_readable(const typename DataItem::fragment_type& item, const typename DataItem::region_type& region) {
	// TODO: implement
}

template<typename DataItem>
void mark_readwriteable(const typename DataItem::fragment_type& item, const typename DataItem::region_type& region) {
	// TODO: implement
}
template<typename DataItem>
void remove_readwriteable(const typename DataItem::fragment_type& item, const typename DataItem::region_type& region) {
	// TODO: implement
}

} // end namespace runtime
} // end namespace allscale

#endif
