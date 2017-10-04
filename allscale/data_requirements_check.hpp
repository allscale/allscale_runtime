#ifndef ALLSCALE_DATA_REQUIREMENTS_CHECK_HPP
#define ALLSCALE_DATA_REQUIREMENTS_CHECK_HPP

#include <iostream>

namespace allscale {
namespace runtime {

#ifdef ALLSCALE_RUNTIME_WITH_DATA_REQUIREMENT_CHECKS

// -- an implementation of the check functionality to be utilized if enabled --

// -- raw interface --

void mark_readable(void* from, void* to);

void remove_readable(void*,void*);

void check_read(void*, std::size_t);

void mark_readwriteable(void* from, void* to);

void remove_readwriteable(void* from,void* to);

void check_write(void*, std::size_t);

bool summarize_access_checks_internal();

inline bool summarize_access_checks() {
	return summarize_access_checks_internal();
}

// -- generic interface --

template<typename DataItem>
void mark_readable(const typename DataItem::fragment_type& fragment, const typename DataItem::region_type& region) {
//	fragment.enumerateMemoryBlocksFor(region,&mark_readable);
}

template<typename DataItem>
void remove_readable(const typename DataItem::fragment_type& fragment, const typename DataItem::region_type& region) {
//	fragment.enumerateMemoryBlocksFor(region,&remove_readable);
}

template<typename T>
const T& check_read(const T& data) {
	check_read((void*)&data,sizeof(T));
	return data;
}

template<typename DataItem>
void mark_readwriteable(const typename DataItem::fragment_type& fragment, const typename DataItem::region_type& region) {
//	fragment.enumerateMemoryBlocksFor(region,&mark_readwriteable);
}

template<typename DataItem>
void remove_readwriteable(const typename DataItem::fragment_type& fragment, const typename DataItem::region_type& region) {
//	fragment.enumerateMemoryBlocksFor(region,&remove_readwriteable);
}

template<typename T>
T& check_write(T& data) {
	check_write((void*)&data,sizeof(T));
	return data;
}


#else

// -- empty implementations of the same interface --

template<typename DataItem>
void mark_readable(const typename DataItem::fragment_type&, const typename DataItem::region_type&) {
	// ignored
}

template<typename DataItem>
void remove_readable(const typename DataItem::fragment_type&, const typename DataItem::region_type&) {
	// ignored
}

template<typename T>
const T& check_read(const T& data) {
	// nothing to check
	return data;
}

template<typename DataItem>
void mark_readwriteable(const typename DataItem::fragment_type&, const typename DataItem::region_type&) {
	// ignored
}

template<typename DataItem>
void remove_readwriteable(const typename DataItem::fragment_type&, const typename DataItem::region_type&) {
	// ignored
}

template<typename T>
T& check_write(T& data) {
	// nothing to check
	return data;
}

inline bool summarize_access_checks() {
	return true; // - all fine
}

#endif


} // end namespace runtime
} // end namespace allscale

#endif
