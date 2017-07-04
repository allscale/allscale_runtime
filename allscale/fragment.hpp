#ifndef ALLSCALE_FRAGMENT_HPP
#define ALLSCALE_FRAGMENT_HPP

#include <allscale/components/fragment.hpp>
#include <hpx/include/serialization.hpp>
#include <memory>
#include <iostream>
#include <hpx/include/lcos.hpp>
#include <allscale/traits/is_fragment.hpp>

#include <iostream>
#include <memory>
#include <utility>

namespace allscale {

    struct fragment_base{
        virtual ~fragment_base(){}
    };


template<typename Region, typename T>
struct fragment : /*hpx::components::client_base<fragment<Region,T>,
                    components::fragment<Region,T> >,*/ fragment_base
{
public:
	using value_type = T;
	using region_type = Region;
	/*
	using base_type = hpx::components::client_base<fragment<Region,T>,
             components::fragment<Region,T> >;
 	 */
	//using future_type = hpx::future<DataItemDescription>;


	fragment(int k)
	{

	}

	fragment()
		{

		}

	/*
	fragment() : base_type(
				hpx::new_<components::fragment<Region,T> >(hpx::find_here())){
			HPX_ASSERT(this->valid());
		}



	fragment(hpx::future<hpx::id_type> && id) : base_type(std::move(id))
	{
		HPX_ASSERT(this->valid());

	}

	fragment(hpx::id_type const& id) : base_type(id) {
		HPX_ASSERT(this->valid());
	}

	fragment(Region const& r, T t) : base_type(
			hpx::new_<components::fragment<Region,T> >(hpx::find_here()))
	{

		T *tmp = new T(t);
		ptr_ = std::shared_ptr < T > (tmp);
		region_ = r;
		HPX_ASSERT(this->valid());

	}
*/


	fragment(Region const& r, T t)
		{

			T *tmp = new T(t);
			ptr_ = std::shared_ptr < T > (tmp);
			region_ = r;
//			HPX_ASSERT(this->valid());

		}
	fragment(Region const& r, std::shared_ptr<T> ptr ) :
			region_(r),
			ptr_(ptr)
	{

	}

	fragment(Region const& r) : region_(r), ptr_(std::make_shared<value_type>(value_type(region_.get_elements())))
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

namespace traits {
template<typename R, typename T>
struct is_fragment<::allscale::fragment<R,T>> : std::true_type {
};
}


}
//
//
//#define ALLSCALE_REGISTER_FRAGMENT_TYPE_(Region, T, NAME)            \
//    using NAME =                                                                \
//        ::hpx::components::managed_component<                                   \
//            ::allscale::components::fragment< Region, T >            \
//        >;                                                                      \
//    HPX_REGISTER_COMPONENT(NAME)                                                \
///**/
//
//#define ALLSCALE_REGISTER_FRAGMENT_TYPE(Region, T)                   \
//    ALLSCALE_REGISTER_FRAGMENT_TYPE_(                                           \
//    	Region, \
//		T,                                                                     \
//        BOOST_PP_CAT(BOOST_PP_CAT(fragment, BOOST_PP_CAT(Region, T)), _component_type)                \
//    )                                                                           \
///**/



#endif
