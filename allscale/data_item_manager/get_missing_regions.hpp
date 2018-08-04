
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
        void get_missing_regions(runtime::HierarchyAddress const& addr, Requirement& req)
        {
            using data_item_type = typename Requirement::data_item_type;

            HPX_ASSERT(!req.region.empty());

            if (req.mode == access_mode::ReadOnly)
                return;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr).get(req.ref);

            req.allowance = entry.get_missing_region(req.region);
        }
        template <typename Requirement>
        void get_missing_regions_left(runtime::HierarchyAddress const& addr, Requirement& req)
        {
            using data_item_type = typename Requirement::data_item_type;

            HPX_ASSERT(!req.region.empty());

            if (req.mode == access_mode::ReadOnly)
                return;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr).get(req.ref);

            req.allowance = entry.get_missing_region_left(req.region);
        }
        template <typename Requirement>
        void get_missing_regions_right(runtime::HierarchyAddress const& addr, Requirement& req)
        {
            using data_item_type = typename Requirement::data_item_type;

            HPX_ASSERT(!req.region.empty());

            if (req.mode == access_mode::ReadOnly)
                return;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr).get(req.ref);

            req.allowance = entry.get_missing_region_right(req.region);
        }

        template <typename Requirement, typename RequirementAllocator>
        void get_missing_regions(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator>& reqs)
        {
            for (auto& req: reqs)
            {
                get_missing_regions(addr, req);
            }
        }

        template <typename Requirement, typename RequirementAllocator>
        void get_missing_regions_left(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator>& reqs)
        {
            for (auto& req: reqs)
            {
                get_missing_regions_left(addr, req);
            }
        }

        template <typename Requirement, typename RequirementAllocator>
        void get_missing_regions_right(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator>& reqs)
        {
            for (auto& req: reqs)
            {
                get_missing_regions_right(addr, req);
            }
        }

        template <typename Requirements, std::size_t...Is>
        void get_missing_regions(runtime::HierarchyAddress const& addr, Requirements& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int const sequencer[] = {0, (detail::get_missing_regions(addr, hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;
        }

        template <typename Requirements, std::size_t...Is>
        void get_missing_regions_right(runtime::HierarchyAddress const& addr, Requirements& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int const sequencer[] = {0, (detail::get_missing_regions_right(addr, hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;
        }

        template <typename Requirements, std::size_t...Is>
        void get_missing_regions_left(runtime::HierarchyAddress const& addr, Requirements& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int const sequencer[] = {0, (detail::get_missing_regions_left(addr, hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;
        }
    }

    template <typename Requirements>
    void
    get_missing_regions(runtime::HierarchyAddress const& addr, Requirements& reqs)
    {
        detail::get_missing_regions(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
    template <typename Requirements>
    void
    get_missing_regions_right(runtime::HierarchyAddress const& addr, Requirements& reqs)
    {
        detail::get_missing_regions_right(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
    template <typename Requirements>
    void
    get_missing_regions_left(runtime::HierarchyAddress const& addr, Requirements& reqs)
    {
        detail::get_missing_regions_left(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
