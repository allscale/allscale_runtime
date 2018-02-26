
#ifndef ALLSCALE_TREETURE_LCO_HPP
#define ALLSCALE_TREETURE_LCO_HPP

#include <allscale/treeture_fwd.hpp>

#include <hpx/traits/managed_component_policies.hpp>
#include <hpx/runtime/actions/component_action.hpp>
#include <hpx/runtime/components/server/managed_component_base.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/util/assert.hpp>

#include <boost/intrusive_ptr.hpp>

namespace allscale { namespace detail {

    template <typename T>
    struct treeture_lco :
        public hpx::components::managed_component_base<treeture_lco<T>>
    {
        typedef typename std::conditional<
            std::is_void<T>::value, hpx::util::unused_type, T>::type
            result_type;

        typedef
            typename hpx::traits::promise_remote_result<T>::type
            remote_result;

        typedef treeture_task<T> shared_state_type;

        treeture_lco()
        {
            HPX_ASSERT(false);
        }

        treeture_lco(boost::intrusive_ptr<shared_state_type> shared_state)
          : shared_state_(std::move(shared_state))
        {
            HPX_ASSERT(shared_state_);
        }

        boost::intrusive_ptr<shared_state_type> get_state()
        {
            HPX_ASSERT(shared_state_);
            return shared_state_;
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_lco, get_state);

        treeture<void> get_parent();
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_lco, get_parent);
        treeture<void> get_left_child();
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_lco, get_left_child);
        treeture<void> get_right_child();
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_lco, get_right_child);
        void set_left_child(treeture<void>);
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_lco, set_left_child);
        void set_right_child(treeture<void>);
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_lco, set_right_child);
        void set_children(treeture<void>, treeture<void>);
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_lco, set_children);

        void set_value(remote_result&& result)
        {
            HPX_ASSERT(shared_state_);
            shared_state_->set_value(std::move(result));
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(treeture_lco, set_value);

        result_type get_value()
        {
            HPX_ASSERT(shared_state_);
            return *shared_state_->get_result();
        }

        void finalize()
        {}

    private:
        boost::intrusive_ptr<treeture_task<T>> shared_state_;
    };
}}

#endif
