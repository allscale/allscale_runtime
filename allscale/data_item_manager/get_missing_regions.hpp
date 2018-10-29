
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

            // read only never has unallocated allowances.
            if (req.mode == access_mode::ReadOnly)
                return false;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr.getLayer()).get(req.ref);

            bool unallocated = false;
            req.allowance = entry.get_missing_region(req.region, unallocated);
            return unallocated;
        }

        template <typename Requirement, typename RequirementAllocator>
        bool get_missing_regions(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator>& reqs)
        {
            bool res = false;
            for (auto& req: reqs)
            {
                if (get_missing_regions(addr, req))
                    res = true;
            }
            return res;
        }

        template <typename Requirements, std::size_t...Is>
        bool get_missing_regions(runtime::HierarchyAddress const& addr, Requirements& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            bool res[] = {detail::get_missing_regions(addr, hpx::util::get<Is>(reqs))...};
            for (bool r: res) if(r) return true;
            return false;
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
