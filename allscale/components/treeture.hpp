
#ifndef ALLSCALE_COMPONENTS_TREETURE_HPP
#define ALLSCALE_COMPONENTS_TREETURE_HPP

#include <hpx/include/components.hpp>
#include <hpx/lcos/detail/future_data.hpp>
#include <hpx/traits/future_access.hpp>

#include <boost/intrusive_ptr.hpp>

namespace allscale { namespace components {
    template <typename T>
    struct treeture
      : hpx::components::managed_component_base<treeture<T> >
    {
        typedef hpx::lcos::detail::future_data<T> shared_state;
//         typedef hpx::components::managed_component_base<treeture<T> > base_type;

        treeture()
          : shared_state_(new shared_state())
        {}

        treeture(T t)
          : shared_state_(new shared_state())
        {
            set_value(std::move(t));
        }

        void set_value(T && t)
        {
            shared_state_->set_value(std::move(t));
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture, set_value);

        hpx::future<T> get_future()
        {
            return hpx::traits::future_access<hpx::future<T>>::create(shared_state_);
        }
        HPX_DEFINE_COMPONENT_ACTION(treeture, get_future);

//         using base_type::set_back_ptr;
//         using base_type::get_base_gid;

    private:
        boost::intrusive_ptr<shared_state> shared_state_;
    };
}}


#endif
