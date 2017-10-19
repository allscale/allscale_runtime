#ifndef ALLSCALE_DATA_ITEM_MANAGER_IMPL_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_IMPL_HPP

#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server.hpp>

#include <hpx/include/components.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/detail/yield_k.hpp>

namespace allscale
{
    template <typename DataItemType>
    struct data_item_manager_impl
    {
        typedef server::data_item_server<DataItemType> component_type;
        typedef data_item_server_network<DataItemType> network_type;
        typedef data_item_reference<DataItemType>      data_reference;

		static allscale::lease<DataItemType>
        acquire(const allscale::data_item_requirement<DataItemType>& requirement)
        {
            return get_ptr()->acquire(requirement);
        }

        static typename DataItemType::facade_type
        get(const data_reference& ref)
        {
            // The Data Item Reference has a fragment pointer which can be used
            // to cache accesses to the data item manager to avoid extra locking
            if (ref.fragment == nullptr)
                ref.fragment = &get_ptr()->get(ref);

            HPX_ASSERT(ref.fragment == &get_ptr()->get(ref));

            return ref.fragment->mask();
        }

        static void release(const allscale::lease<DataItemType>& lease)
        {
            return get_ptr()->release(lease);
        }

		static void destroy(const data_reference& ref) {
            return get_ptr()->destroy(ref);
        }

        // This function is called to make sure that the local components
        // are being instantiated...
        static void force_to_exist()
        {
            auto ptr = get_ptr();
            HPX_ASSERT(ptr);
        }

    private:
        typedef hpx::lcos::local::spinlock mutex_type;

        data_item_manager_impl()
          : component_(
                hpx::local_new<component_type>().then(
                    [](hpx::future<hpx::id_type> fid)
                    {
                        hpx::id_type server_id = fid.get();
                        std::size_t rank = hpx::get_locality_id();
                        hpx::register_with_basename(data_item_server_name<DataItemType>::name(), server_id, rank);

                        return hpx::get_ptr<component_type>(server_id);
                    }
                )
          )
        {
//                 typedef typename allscale::server::data_item_server<DataItemType> data_item_server_type;
//                 //CREATE DATA ITEM SERVER INSTANCES ON LOCALITIES
//                 allscale::data_item_server_network<DataItemType> sn;
//                 std::vector < hpx::id_type > localities = hpx::find_all_localities();
//                 std::vector<allscale::data_item_server<DataItemType>> result;
//                 for (auto& loc : localities)
//                 {
//                     allscale::data_item_server<DataItemType> server(
//                         hpx::components::new_ < data_item_server_type > (loc).get());
//                     result.push_back(server);
//                 }
//                 sn.servers = result;
//                 for( auto& server : result)
//                 {
//                     server.set_network(sn);
//                 }
//
//
//                 return sn;
        }

        static component_type* get_ptr()
        {
            static data_item_manager_impl impl;
            return impl.component_.get().get();
        }

        hpx::shared_future<std::shared_ptr<component_type>> component_;
    };
}

#endif
