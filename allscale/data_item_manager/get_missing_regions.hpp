
#ifndef ALLSCALE_DATA_ITEM_MANAGER_GET_MISSING_REGIONS_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_GET_MISSING_REGIONS_HPP

#include <allscale/lease.hpp>
#include <allscale/api/core/data.h>
#include <allscale/data_item_manager/index_service.hpp>

#include <hpx/util/annotated_function.hpp>

#include <hpx/include/actions.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/util/tuple.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        template <typename Requirement>
        bool get_missing_regions(runtime::HierarchyAddress const& addr, Requirement& req)
        {
            using data_item_type = typename Requirement::data_item_type;

            HPX_ASSERT(!req.region.empty());

            if (req.mode == access_mode::ReadOnly)
                return false;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr).get(req.ref);

            req.allowance = entry.get_managed_unallocated(req.region);

            if (!req.allowance.empty())
            {
                return true;
            }
            req.allowance = entry.get_missing_region(req.region);

            return false;
        }

        template <typename Requirement, typename RequirementAllocator>
        bool get_missing_regions(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator>& reqs)
        {
            bool res = true;
            for (auto& req: reqs)
            {
                if(!get_missing_regions(addr, req))
                {
                    res = false;
                }
            }
            return res;
        }

        template <typename Requirements, std::size_t...Is>
        bool get_missing_regions(runtime::HierarchyAddress const& addr, Requirements& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            bool results[] = {detail::get_missing_regions(addr, hpx::util::get<Is>(reqs))...};
            for (bool r : results)
            {
                if (!r) return false;
            }
            return true;
        }
    }

    template <typename Requirements>
    bool
    get_missing_regions(runtime::HierarchyAddress const& addr, Requirements& reqs)
    {
        return detail::get_missing_regions(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
