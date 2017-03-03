#ifndef ALLSCALE_FRAGMENT_HPP
#define ALLSCALE_FRAGMENT_HPP

#include <hpx/include/serialization.hpp>
#include <memory>
#include <iostream>
namespace allscale {
template<typename Region, typename T>
struct fragment {
public:
	using value_type = T;
	using region_type = Region;

	fragment() {
	}

	fragment(Region const& r, T t) :
			region_(r) {
		T *tmp = new T(t);
		ptr_ = std::shared_ptr < T > (tmp);
	}

	fragment(Region const& r, std::shared_ptr<T> ptr ) :
			region_(r),
			ptr_(ptr)
	{

	}

	void resize(fragment const& fragment, Region const& region) {
	}

	fragment mask(fragment const& fragment, Region const& region) {
	}

	void insert(fragment & destination, fragment const& source,
			Region const& region) {
	}

	template<typename Archive>
	void save(Archive& ar, fragment const& fragment) {
	}

	template<typename Archive>
	void serialize(Archive & ar, unsigned) {
		ar & region_;
		ar & ptr_;
	}

	template<typename Archive>
	void load(Archive& ar, fragment & fragment) {
	}

	Region region_;
	std::shared_ptr<T> ptr_;
};
}

#endif
