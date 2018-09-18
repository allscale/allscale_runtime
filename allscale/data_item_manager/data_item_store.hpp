
#ifndef ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_STORE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_STORE_HPP

#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_manager/data_item.hpp>
#include <allscale/util/readers_writers_mutex.hpp>

#include <hpx/runtime/naming/id_type.hpp>

#include <boost/thread.hpp>

#include <unordered_map>

namespace allscale { namespace data_item_manager {

    namespace detail
    {
        void tls_cache(hpx::naming::gid_type const& id, void *);
        void *tls_cache(hpx::naming::gid_type const& id);

        template <typename DataItem>
        DataItem* tls_cache(hpx::naming::gid_type const& id)
        {
            return reinterpret_cast<DataItem*>(tls_cache(id));
        }
    }

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
        using mutex_type = allscale::util::readers_writers_mutex;
        static mutex_type mtx;
        static std::unordered_map<id_type, data_item_type> store_;

        auto dmp = detail::tls_cache<data_item_type>(id);
        if (dmp != nullptr)
            return *dmp;

        {
            std::unique_lock<mutex_type> ll(mtx);

            auto it = store_.find(id);
            if (it == store_.end())
            {
                dmp = &store_[id];
                dmp->id = id;
                detail::tls_cache(id, dmp);
                return *dmp;
            }

            detail::tls_cache(id, &it->second);

            return it->second;
        }
    }

    static data_item_type& lookup(data_item_reference const& ref)
    {
        return lookup(ref.id());
    }
};

}}

#endif
