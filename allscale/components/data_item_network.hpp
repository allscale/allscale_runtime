#ifndef ALLSCALE_DATA_ITEM_SERVER_NETWORK
#define ALLSCALE_DATA_ITEM_SERVER_NETWORK


// #include <hpx/runtime/serialization/serialize.hpp>
// #include <hpx/runtime/serialization/vector.hpp>
// #include <hpx/runtime/serialization/input_archive.hpp>
// #include <hpx/runtime/serialization/output_archive.hpp>


// #include <allscale/data_item_server.hpp>

#include <hpx/runtime/basename_registration.hpp>

namespace allscale { namespace components {

    struct data_item_network {
    public:
        data_item_network(const char* base)
          : base_(base)
        {}

        template <typename Callback>
        void apply(std::size_t rank, Callback&&cb)
        {
            hpx::id_type id;
            {
                std::unique_lock<mutex_type> l(mtx_);
                auto it = servers_.find(rank);
                // If we couldn't find it in the map, we resolve the name.
                // This is happening asynchronously. Once the name was resolved,
                // the callback is apply and the id is being put in the network map.
                //
                // If we are able to locate the rank in our map, we just go ahead and
                // apply the callback.
                if (it == servers_.end())
                {
                    l.unlock();
                    hpx::find_from_basename(base_, rank).then(
                        hpx::util::bind(
                            hpx::util::one_shot(
                            [this, rank](hpx::future<hpx::id_type> fut,
                                typename hpx::util::decay<Callback>::type cb)
                            {
                                hpx::id_type id = fut.get();
                                cb(id);
                                {
                                    std::unique_lock<mutex_type> l(mtx_);
                                    // We need to search again in case of concurrent
                                    // lookups.
                                    auto it = servers_.find(rank);
                                    if (it == servers_.end())
                                    {
                                        servers_.insert(std::make_pair(rank, id));
                                    }
                                }
                            }), hpx::util::placeholders::_1, std::forward<Callback>(cb)));
                    return;
                }
                id = it->second;
            }

            HPX_ASSERT(id);

            cb(id);
        }

    private:
        const char* base_;

        typedef hpx::lcos::local::spinlock mutex_type;
        mutex_type mtx_;
        std::unordered_map<std::size_t, hpx::id_type> servers_;
    };
}}

#endif
