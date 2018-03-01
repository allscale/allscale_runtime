
#ifndef ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_STORE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_STORE_HPP

#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_manager/data_item.hpp>

#include <hpx/runtime/naming/id_type.hpp>

#include <map>

namespace allscale { namespace data_item_manager {

template<typename DataItemType>
struct data_item_store
{
    using id_type = hpx::naming::gid_type;
    using data_item_type = data_item<DataItemType>;
    using data_item_reference = ::allscale::data_item_reference<DataItemType>;

    static_assert(!std::is_same<DataItemType, data_item<DataItemType>>::value,
        "Wrong type ...");

    static void destroy(data_item_reference const& ref)
    {
        HPX_ASSERT(false);
//         auto l = lock();
//         auto store_it = store().find(ref.id());
//         HPX_ASSERT(store_it != store().end());
//         store().erase(ref.id());
        // FIXME: broadcast to destroy everywhere...
    }


    static data_item_type& lookup(id_type const& id)
    {
        static hpx::lcos::local::spinlock mtx;
        static std::unordered_map<id_type, data_item_type> store_;
        std::unique_lock<hpx::lcos::local::spinlock> l(mtx);

        return store_[id];
    }

    static data_item_type& lookup(data_item_reference const& ref)
    {
        return lookup(ref.id());
    }
};

}}

#endif
