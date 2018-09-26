
#ifndef ALLSCALE_DATA_ITEM_MANAGER_FRAGMENT_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_FRAGMENT_HPP

#include <allscale/data_item_manager/data_item.hpp>
#include <allscale/data_item_manager/shared_data.hpp>

namespace allscale { namespace data_item_manager {
    template <typename Ref, typename DataItem, typename Lock>
    typename DataItem::fragment_type& fragment(const Ref& ref, allscale::data_item_manager::data_item<DataItem>& item, Lock& l)
    {
        using mutex_type = typename data_item_store<DataItem>::data_item_type::mutex_type;
        if (item.fragment == nullptr)
        {
            hpx::util::unlock_guard<Lock> ul(l);
            typename DataItem::shared_data_type shared_data_ = shared_data(ref);
            {
                std::unique_lock<mutex_type> ll(item.mtx);
                if (item.fragment == nullptr)
                {
                    using fragment_type = typename DataItem::fragment_type;
                    item.fragment.reset(new fragment_type(std::move(shared_data_)));
                }
            }
        }
        HPX_ASSERT(item.fragment);
        return *item.fragment;
    }
    template <typename Ref, typename DataItem>
    typename DataItem::fragment_type& fragment(const Ref& ref, allscale::data_item_manager::data_item<DataItem>& item)
    {
        using mutex_type = typename data_item_store<DataItem>::data_item_type::mutex_type;
        std::unique_lock<mutex_type> l(item.mtx);
        return fragment(ref, item, l);
    }

    template <typename DataItem>
    typename DataItem::fragment_type& fragment(const allscale::data_item_reference<DataItem>& ref)
    {
        return fragment(ref, data_item_store<DataItem>::lookup(ref));
    }
}}

#endif
