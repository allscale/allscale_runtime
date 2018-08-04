
#ifndef ALLSCALE_DATA_ITEM_MANAGER_MARK_WRITE_REQUIREMENTS_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_MARK_WRITE_REQUIREMENTS_HPP

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
        void mark_write_requirements(runtime::HierarchyAddress const& addr, Requirement const& req)
        {
            using data_item_type = typename Requirement::data_item_type;

            HPX_ASSERT(!req.region.empty());

            if (req.mode == access_mode::ReadOnly)
                return;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr).get(req.ref);

            entry.mark_region(req.allowance);
        }

        template <typename Requirement, typename RequirementAllocator>
        void mark_write_requirements(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator> const& reqs)
        {
            for (auto& req: reqs)
            {
                mark_write_requirements(addr, req);
            }
        }

        template <typename Requirements, std::size_t...Is>
        void mark_write_requirements(runtime::HierarchyAddress const& addr, Requirements const& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int const sequencer[] = {0, (detail::mark_write_requirements(addr, hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;
        }
    }

    template <typename Requirements>
    void
    mark_write_requirements(runtime::HierarchyAddress const& addr, Requirements const& reqs)
    {
        detail::mark_write_requirements(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
