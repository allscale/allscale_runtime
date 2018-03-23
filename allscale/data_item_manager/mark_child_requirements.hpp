
#ifndef ALLSCALE_DATA_ITEM_MANAGER_MARK_CHILD_REQUIREMENTS_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_MARK_CHILD_REQUIREMENTS_HPP

#include <allscale/lease.hpp>
#include <allscale/api/core/data.h>
#include <allscale/data_item_manager/data_item_store.hpp>
#include <allscale/data_item_manager/fragment.hpp>
#include <allscale/data_item_manager/data_item_view.hpp>
#include <allscale/data_item_manager/location_info.hpp>

#include <hpx/util/annotated_function.hpp>

#include <hpx/include/actions.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/util/function.hpp>
#include <hpx/util/tuple.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        template <typename Requirement>
        struct register_owned_fun
        {
            using data_item_type = typename Requirement::data_item_type;
            using ref_type = typename Requirement::ref_type;
            using region_type = typename Requirement::region_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            ref_type ref;
            region_type assigned;

            void operator()()
            {
                auto& item = data_item_store<data_item_type>::lookup(ref);
                std::unique_lock<mutex_type> l(item.region_mtx);
                HPX_ASSERT(item.owned_region.empty());
                item.owned_region = std::move(assigned);
            }

            template <typename Archive>
            void serialize(Archive& ar, unsigned)
            {
                ar & ref;
                ar & assigned;
            }
        };

        template <typename Requirement>
        void mark_child_requirement(Requirement const& req, std::size_t dest_id,
            std::vector<hpx::util::function<void()>>& register_owned)
        {
            using data_item_type = typename Requirement::data_item_type;
            using region_type = typename Requirement::region_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;
            auto& item = data_item_store<data_item_type>::lookup(req.ref);

            std::unique_lock<mutex_type> l(item.region_mtx);

            region_type& this_child_region = item.child_regions[dest_id];
            if (!this_child_region.empty())
            {
//                 HPX_ASSERT(!region_type(req.region, this_child_region));
                return;
            }



//             for (auto& child: item.child_regions)
//             {
//                 if (child.first != dest_id)
//                 {
//                     allowed = region_type::difference(allowed, child.second);
//                 }
//             }
            if (item.owned_region.empty())
            {
                HPX_ASSERT(hpx::get_locality_id() == 0);
                item.owned_region = fragment(req.ref, item, l).getTotalSize();
            }
            item.owned_region = region_type::difference(item.owned_region, req.region);
//             this_child_region = std::move(allowed);
            this_child_region = req.region;
            HPX_ASSERT(!allscale::api::core::isSubRegion(this_child_region, item.owned_region));
            register_owned.push_back(register_owned_fun<Requirement>{req.ref, req.region});
        }

        template <typename Requirement, typename RequirementAllocator>
        void mark_child_requirement(
            std::vector<Requirement, RequirementAllocator> const& reqs, std::size_t dest_id,
            std::vector<hpx::util::function<void()>>& register_owned)
        {
            for (auto const& req: reqs)
            {
                detail::mark_child_requirement(req, dest_id, register_owned);
            }
        }

        template <typename Requirements, std::size_t...Is>
        void mark_child_requirements(Requirements const& reqs, std::size_t dest_id,
            std::vector<hpx::util::function<void()>>& register_owned,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int sequencer[] = {
                (detail::mark_child_requirement(
                    hpx::util::get<Is>(reqs), dest_id, register_owned), 0)...
            };
            (void)sequencer;
        }
    }

    template <typename Requirements>
    void mark_child_requirements(Requirements const& reqs, std::size_t dest_id,
        std::vector<hpx::util::function<void()>>& register_owned)
    {
        detail::mark_child_requirements(reqs, dest_id, register_owned,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
