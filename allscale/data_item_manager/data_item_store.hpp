
#ifndef ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_STORE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_STORE_HPP

#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_manager/data_item.hpp>
#include <allscale/util/readers_writers_mutex.hpp>

#include <hpx/runtime/naming/id_type.hpp>

#include <boost/thread.hpp>

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
        using mutex_type = allscale::util::readers_writers_mutex;
        static mutex_type mtx;
        static std::unordered_map<id_type, data_item_type> store_;

        // A thread local cache to avoid locking alltogether.
        thread_local static std::unordered_map<id_type, data_item_type*> tls_store_;

        auto jt = tls_store_.find(id);
        if (jt != tls_store_.end())
            return *jt->second;

        {
            boost::shared_lock<mutex_type> l(mtx);

            auto it = store_.find(id);
            if (it == store_.end())
            {
                l.unlock();
                std::unique_lock<mutex_type> ll(mtx);
                tls_store_[id] = &store_[id];
                return store_[id];
            }

            tls_store_[id] = &it->second;

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
