
#include <allscale/detail/work_item_impl_base.hpp>

namespace allscale { namespace detail {
    void work_item_impl_base::set_this_id(machine_config const& mconfig, bool is_first)
    {
        id_.set(this, mconfig, this->can_split() || !is_first);
    }

    this_work_item::id const& work_item_impl_base::id() const
    {
        return id_;
    }
}}
