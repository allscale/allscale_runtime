#ifndef ALLSCALE_DATA_ITEM_MANAGER
#define ALLSCALE_DATA_ITEM_MANAGER

#include <allscale/config.hpp>
#include <allscale/data_item_manager/acquire.hpp>
#include <allscale/data_item_manager/data_item_store.hpp>
#include <allscale/data_item_manager/fragment.hpp>
#include <allscale/data_item_manager/shared_data.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/work_item_description.hpp>
#include <allscale/spawn_fwd.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/lease.hpp>
#include "allscale/utils/serializer.h"
#include "allscale/api/user/data/binary_tree.h"

#include <hpx/runtime/components/new.hpp>
#include <hpx/util/detail/yield_k.hpp>

#include <type_traits>

namespace allscale { namespace data_item_manager {
        template <typename DataItem>
        struct initializer
        {
            static void call(data_item_reference<DataItem> const& ref)
            {
            }
        };

        template <typename T, std::size_t depth>
        struct initializer<
            allscale::api::user::data::StaticBalancedBinaryTree<T, depth>
        >
        {
            using data_item_type =
                allscale::api::user::data::StaticBalancedBinaryTree<T, depth>;
            struct tree_init_name {
                static const char* name() { return "tree_init"; }
            };
            struct tree_init_split;
            struct tree_init_process;
            struct tree_init_can_split;

            using tree_init = allscale::work_item_description<
                void,
                tree_init_name,
                allscale::do_serialization,
                tree_init_split,
                tree_init_process,
                tree_init_can_split
            >;

            struct tree_init_can_split
            {
                template <typename Closure>
                static bool call(Closure const& c)
                {
                    auto begin = hpx::util::get<1>(c);
                    auto end = hpx::util::get<2>(c);

                    return end - begin > 2;
                }
            };

            struct tree_init_split {
                template <typename Closure>
                static hpx::future<void> execute(Closure const& c)
                {
                    auto data = hpx::util::get<0>(c);
                    auto begin = hpx::util::get<1>(c);
                    auto end = hpx::util::get<2>(c);

                    auto mid = begin + (end - begin)/ 2;

                    auto l = allscale::spawn<tree_init>(data, begin, mid);
                    auto r = allscale::spawn<tree_init>(data, mid, end);

                    return hpx::when_all(l.get_future(), r.get_future());
                }

                static constexpr bool valid = true;
            };

            struct tree_init_process {
                template <typename Closure>
                static void execute(Closure const& c)
                {
                    // do nothing ...
                }

                template <typename Closure>
                static hpx::util::tuple<
                    allscale::data_item_requirement<data_item_type>
                >
                get_requirements(Closure const& c)
                {
                    auto data = hpx::util::get<0>(c);
                    auto begin = hpx::util::get<1>(c);
                    auto end = hpx::util::get<2>(c);

                    using region_type = typename data_item_type::region_type;

                    region_type region;

            //         if (begin == 0) region = region_type::root();

                    for (auto i = begin; i < end; ++i)
                    {
                        region = region_type::merge(region, region_type::subtree(i));
                    }

                    return hpx::util::make_tuple(
                        allscale::createDataItemRequirement(
                            data, region,
                            allscale::access_mode::ReadWrite
                        )
                    );
                }
                static constexpr bool valid = true;
            };
            static void call(data_item_reference<data_item_type> const& ref)
            {
                std::cerr << " (Default distributing Binary Tree) ";
                auto end = data_item_type::region_type::num_leaf_trees;
                allscale::spawn_first<tree_init>(ref, 0, end).wait();
            }
        };


        template <typename DataItem, typename...Args>
        allscale::data_item_reference<DataItem>
        create(Args&&...args)
        {
            typedef typename DataItem::shared_data_type shared_data_type;

            auto ref = data_item_reference<DataItem>(
                    hpx::local_new<allscale::detail::id_holder>(
                        [](hpx::naming::gid_type const& id)
                        {
//                             data_item_manager_impl<DataItem>::get_ptr()->destroy(id);
                        }
                    )
                );
            auto &item = data_item_store<DataItem>::lookup(ref);
            item.shared_data.reset(new shared_data_type(std::forward<Args>(args)...));
            initializer<DataItem>::call(ref);
            return ref;
        }

        template<typename DataItem>
        typename DataItem::facade_type
        get(const allscale::data_item_reference<DataItem>& ref)
        {
            auto hint = ref.getFragmentHint();
            if (hint) {
                return hint->mask();
            }
            return ref.setFragmentHint(&fragment(ref))->mask();
        }

        template <typename T>
        void release(T&&)
        {
        }

//         template<typename DataItem>
// 		void release(const allscale::lease<DataItem>& lease)
//         {
// //             if (lease.mode == access_mode::Invalid)
// //                 return;
// //             data_item_manager_impl<DataItem>::release(lease);
//         }
//
//         template<typename DataItem>
// 		void release(const std::vector<allscale::lease<DataItem>>& lease)
//         {
// //             for(auto const& l: lease)
// //                 if (l.mode == access_mode::Invalid) return;
// //
// //             data_item_manager_impl<DataItem>::release(lease);
//         }

        template<typename DataItem>
		void destroy(const data_item_reference<DataItem>& ref)
        {
//             data_item_manager_impl<DataItem>::destroy(ref);
        }
}}//end namespace allscale

#define REGISTER_DATAITEMSERVER_DECLARATION(type)                               \
    HPX_REGISTER_ACTION_DECLARATION(                                            \
        allscale::data_item_manager::detail::transfer_action<type>,             \
        HPX_PP_CAT(transfer_action_, type))                                     \
/**/

#define REGISTER_DATAITEMSERVER(type)                                           \
    HPX_REGISTER_ACTION(                                                        \
        allscale::data_item_manager::detail::transfer_action<type>,             \
        HPX_PP_CAT(transfer_action_, type))                                     \
/**/

#endif
