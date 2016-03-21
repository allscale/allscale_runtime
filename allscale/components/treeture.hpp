
#ifndef ALLSCALE_COMPONENTS_TREETURE_HPP
#define ALLSCALE_COMPONENTS_TREETURE_HPP

#include <hpx/include/components.hpp>

namespace allscale { namespace components {
    template <typename T>
    struct treeture
      : hpx::components::component_base<treeture<T> >
    {
        treeture()
        {}

        treeture(T t)
        {
            set_value(std::move(t));
        }

        void set_value(T && t)
        {
            promise_.set_value(std::move(t));
        }
        HPX_DEFINE_COMPONENT_ACTION(treeture, set_value);

        hpx::future<T> get_future()
        {
            return promise_.get_future();
        }
        HPX_DEFINE_COMPONENT_ACTION(treeture, get_future);

    private:
        hpx::lcos::local::promise<T> promise_;
    };
}}


#endif
