
#ifndef ALLSCALE_COMPONENTS_TREETURE_HPP
#define ALLSCALE_COMPONENTS_TREETURE_HPP

#include <hpx/include/components.hpp>
#include <hpx/lcos/detail/future_data.hpp>
#include <hpx/traits/future_access.hpp>

#include <boost/intrusive_ptr.hpp>

namespace allscale { namespace components {

    struct treeture_base
      : hpx::components::abstract_managed_component_base<treeture_base>
    {
        typedef hpx::lcos::local::spinlock mutex_type;

        treeture_base()
        {
            HPX_ASSERT(false);
        }

        treeture_base(hpx::id_type parent)
          : parent_(std::move(parent))
        {
        }

        virtual hpx::id_type get_id() const=0;

        void set_child(std::size_t idx, hpx::id_type child)
        {
            std::lock_guard<mutex_type> lk(children_mtx_);
            if (children_.size() <= idx)
                children_.resize(idx + 1);
            children_[idx] = std::move(child);
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_base, set_child);

        hpx::id_type parent()
        {
            return parent_;
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_base, parent);

        void attach_promise(hpx::id_type pid, hpx::naming::address addr)
        {
            this->attach_promise_virt(std::move(pid), std::move(addr));
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_base, attach_promise);
        virtual void attach_promise_virt(hpx::id_type pid, hpx::naming::address addr)=0;

        hpx::id_type get_child(std::size_t idx)
        {
            {
                std::lock_guard<mutex_type> lk(children_mtx_);
                if (children_.size() > idx)
                    return children_[idx];
            }
            return this->get_id();
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_base, get_child);

        hpx::id_type get_left_neighbor()
        {
            if (parent_)
                return get_child_action()(parent_, 1);
            return this->get_id();
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_base, get_left_neighbor);

        hpx::id_type get_right_neighbor()
        {
            if (parent_)
                return get_child_action()(parent_, 0);
            return this->get_id();
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_base, get_right_neighbor);

        hpx::id_type parent_;
        mutex_type children_mtx_;
        std::vector<hpx::id_type> children_;
    };

    template <typename T>
    struct treeture
      : treeture_base,
        hpx::components::managed_component_base<treeture<T> >
    {
        typedef hpx::lcos::detail::future_data<T> shared_state;

        typedef treeture<T> type_holder;
        typedef treeture_base base_type_holder;
        typedef hpx::components::managed_component<treeture<T> > wrapping_type;

        typedef hpx::components::managed_component_base<treeture<T> > base_type;


        treeture()
        {
            HPX_ASSERT(false);
        }

        treeture(hpx::id_type parent)
          : treeture_base(std::move(parent)),
            shared_state_(new shared_state())
        {
        }

        treeture(T t, hpx::id_type parent)
          : treeture(std::move(parent))
        {
            set_value(std::move(t));
        }

        hpx::id_type get_id() const
        {
            return this->base_type::get_id();
        }

        void set_value(T && t)
        {
            shared_state_->set_value(std::move(t));
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture, set_value);

        void attach_promise_virt(hpx::id_type pid, hpx::naming::address addr)
        {
            shared_state_->set_on_completed(
                [pid, addr, this]() mutable {
                    T result = *shared_state_->get_result();
                    hpx::set_lco_value(pid, std::move(addr), std::move(result));
                });
        }

//         using base_type::set_back_ptr;
//         using base_type::get_base_gid;

    private:
        boost::intrusive_ptr<shared_state> shared_state_;
    };
}}

#endif
