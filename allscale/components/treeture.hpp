
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
        typedef hpx::lcos::local::spinlock mutex_type;
        typedef hpx::lcos::detail::future_data<T> shared_state;
//         typedef hpx::components::managed_component_base<treeture<T> > base_type;

        treeture()
        {
            HPX_ASSERT(false);
        }

        treeture(hpx::id_type parent)
          : shared_state_(new shared_state()),
            parent_(std::move(parent))
        {
        }

        treeture(T t, hpx::id_type parent)
          : treeture(std::move(parent))
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
//         HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture, get_future);
        HPX_DEFINE_COMPONENT_ACTION(treeture, get_future);

        hpx::id_type parent()
        {
            return parent_;
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture, parent);

        void set_child(std::size_t idx, hpx::id_type child)
        {
            std::lock_guard<mutex_type> lk(children_mtx_);
            if (children_.size() <= idx)
                children_.resize(idx + 1);
            children_[idx] = child;
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture, set_child);

        hpx::id_type get_child(std::size_t idx)
        {
            {
                std::lock_guard<mutex_type> lk(children_mtx_);
                if (children_.size() > idx)
                    return children_[idx];
            }
            return this->get_id();
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture, get_child);

        hpx::id_type get_left_neighbor()
        {
            if (parent_)
                return get_child_action()(parent_, 1);
            return this->get_id();
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture, get_left_neighbor);

        hpx::id_type get_right_neighbor()
        {
            if (parent_)
                return get_child_action()(parent_, 0);
            return this->get_id();
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture, get_right_neighbor);

//         using base_type::set_back_ptr;
//         using base_type::get_base_gid;

    private:
        boost::intrusive_ptr<shared_state> shared_state_;

        hpx::id_type parent_;
        mutex_type children_mtx_;
        std::vector<hpx::id_type> children_;
    };
}}


#endif
