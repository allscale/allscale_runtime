#ifndef ALLSCALE_DATA_ITEM_MANAGER_IMPL_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_IMPL_HPP

#include <allscale/data_item_reference.hpp>
#include <allscale/components/fragment_view.hpp>
#include <allscale/components/data_item.hpp>

#include <hpx/include/components.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/detail/yield_k.hpp>

namespace allscale
{
    template <typename DataItemType>
    struct data_item_registry
    {
        data_item_registry();

        data_item_registry& instantiate()
        {
            return *this;
        }
    };

    template <typename DataItemType>
    struct data_item_manager_impl
    {
        typedef components::data_item<DataItemType> component_type;
        typedef data_item_reference<DataItemType>   data_reference;

        virtual ~data_item_manager_impl()
        {
            registry_.instantiate();
        }

		static hpx::future<allscale::lease<DataItemType>>
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

        static component_type* get_ptr()
        {
            static data_item_manager_impl impl;
            return impl.component_.get().get();
        }

        static data_item_registry<DataItemType> registry_;

    private:
        typedef hpx::lcos::local::spinlock mutex_type;

        data_item_manager_impl()
          : component_(
                hpx::local_new<component_type>(hpx::get_locality_id()).then(
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
        }

        hpx::shared_future<std::shared_ptr<component_type>> component_;

        template <typename T>
        friend struct data_item_registry;
    };

    template <typename DataItemType>
    data_item_registry<DataItemType>::data_item_registry()
    {
        hpx::register_startup_function(
            []()
            {
                auto ptr = data_item_manager_impl<DataItemType>::get_ptr();
                if (!ptr)
                {
                    std::cerr << "unable to create data item server...\n";
                    std::abort();
                    return;
                }
                std::cerr << "Created " <<
                    data_item_server_name<DataItemType>::name() << '/' << ptr->rank_ << '\n';
            });
    }

    template <typename DataItemType>
    allscale::data_item_registry<DataItemType>
        allscale::data_item_manager_impl<DataItemType>::registry_;
}

#endif
