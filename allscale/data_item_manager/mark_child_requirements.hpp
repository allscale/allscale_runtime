
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
#include <hpx/util/tuple.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        template <typename Requirement>
        void mark_child_requirement(Requirement const& req, std::size_t dest_id)
        {
            using data_item_type = typename Requirement::data_item_type;
            using region_type = typename Requirement::region_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;
            auto& item = data_item_store<data_item_type>::lookup(req.ref);

            std::unique_lock<mutex_type> l(item.region_mtx);

            region_type& this_child_region = item.child_regions[dest_id];
            if (!this_child_region.empty())
            {
//                 HPX_ASSERT(allscale::api::core::isSubRegion(allowed, this_child_region));
                return;
            }

            region_type allowed = region_type::difference(req.region, item.owned_region);
            for (auto& child: item.child_regions)
            {
                if (child.first != dest_id)
                {
                    allowed = region_type::difference(allowed, child.second);
                }
            }
            this_child_region = std::move(allowed);//region_type::merge(allowed, this_child_region);
        }

        template <typename Requirement, typename RequirementAllocator>
        void mark_child_requirement(
            std::vector<Requirement, RequirementAllocator> const& reqs, std::size_t dest_id)
        {
            for (auto const& req: reqs)
            {
                detail::mark_child_requirement(req, dest_id);
            }
        }

        template <typename Requirements, std::size_t...Is>
        void mark_child_requirements(Requirements const& reqs, std::size_t dest_id,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int sequencer[] = {
                (detail::mark_child_requirement(hpx::util::get<Is>(reqs), dest_id), 0)...
            };
            (void)sequencer;
        }
    }

    template <typename Requirements>
    void mark_child_requirements(Requirements const& reqs, std::size_t dest_id)
    {
        detail::mark_child_requirements(reqs, dest_id,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
