
#ifndef ALLSCALE_DATA_ITEM_MANAGER_CHECK_WRITE_REQUIREMENTS_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_CHECK_WRITE_REQUIREMENTS_HPP

#include <allscale/lease.hpp>
#include <allscale/api/core/data.h>
#include <allscale/data_item_manager/index_service.hpp>

#include <hpx/util/annotated_function.hpp>

#include <hpx/plugins/parcel/coalescing_message_handler_registration.hpp>

#include <hpx/include/actions.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/util/tuple.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        template <typename Requirement>
        bool check_write_requirements(runtime::HierarchyAddress const& addr, Requirement const& req)
        {
            using data_item_type = typename Requirement::data_item_type;
            using lease_type = allscale::lease<data_item_type>;
            using region_type = typename data_item_type::region_type;

            HPX_ASSERT(!req.region.empty());

            if (req.mode == access_mode::ReadOnly)
                return true;


            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr).get(req.ref);

            return entry.get_missing_region(req.region).empty();
        }

        template <typename Requirement, typename RequirementAllocator>
        bool check_write_requirements(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator> const& reqs)
        {
            for (auto& req: reqs)
            {
                if(!check_write_requirements(addr, req))
                {
                    return false;
                }
            }
            return true;
        }

        template <typename Requirements, std::size_t...Is>
        bool check_write_requirements(runtime::HierarchyAddress const& addr, Requirements const& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            bool res[] = {detail::check_write_requirements(addr, hpx::util::get<Is>(reqs))...};
            for (bool r: res) if(!r) return false;
            return true;
        }
    }

    template <typename Requirements>
    bool
    check_write_requirements(runtime::HierarchyAddress const& addr, Requirements const& reqs)
    {
        return detail::check_write_requirements(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
