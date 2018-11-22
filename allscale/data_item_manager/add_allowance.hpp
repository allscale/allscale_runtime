
#ifndef ALLSCALE_DATA_ITEM_MANAGER_ADD_ALLOWANCE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_ADD_ALLOWANCE_HPP

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
        void add_allowance(runtime::HierarchyAddress const& addr, Requirement const& req)
        {
            using data_item_type = typename Requirement::data_item_type;
            using lease_type = allscale::lease<data_item_type>;
            using region_type = typename data_item_type::region_type;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr.getLayer()).get(req.ref);
            HPX_ASSERT(!req.region.empty());

            bool isLeaf = addr.isLeaf();
            if (req.mode == access_mode::ReadWrite)
            {
                entry.add_full(req.allowance);
                if (isLeaf)
                    entry.resize_fragment(req, req.allowance, true);
            }
            else
            {
                if (isLeaf)
                    entry.resize_fragment(req, req.region, false);
            }
        }

        template <typename Requirement>
        void add_allowance_left(runtime::HierarchyAddress const& addr, Requirement& req)
        {
            using data_item_type = typename Requirement::data_item_type;
            using lease_type = allscale::lease<data_item_type>;
            using region_type = typename data_item_type::region_type;

            HPX_ASSERT(!req.region.empty());

            if (req.mode == access_mode::ReadOnly)
                return;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr.getLayer()).get(req.ref);

            req.allowance = entry.add_left(req.allowance, req.region);
        }

        template <typename Requirement>
        void add_allowance_right(runtime::HierarchyAddress const& addr, Requirement& req)
        {
            using data_item_type = typename Requirement::data_item_type;
            using lease_type = allscale::lease<data_item_type>;
            using region_type = typename data_item_type::region_type;

            HPX_ASSERT(!req.region.empty());

            if (req.mode == access_mode::ReadOnly)
                return;

            auto& entry =
                runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr.getLayer()).get(req.ref);

            req.allowance = entry.add_right(req.allowance, req.region);
        }

        template <typename Requirement, typename RequirementAllocator>
        void add_allowance(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator> const& reqs)
        {
            for (auto& req: reqs)
            {
                add_allowance(addr, req);
            }
        }

        template <typename Requirement, typename RequirementAllocator>
        void add_allowance_left(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator>& reqs)
        {
            for (auto& req: reqs)
            {
                add_allowance_left(addr, req);
            }
        }

        template <typename Requirement, typename RequirementAllocator>
        void add_allowance_right(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator>& reqs)
        {
            for (auto& req: reqs)
            {
                add_allowance_right(addr, req);
            }
        }

        template <typename Requirements, std::size_t...Is>
        void add_allowance(runtime::HierarchyAddress const& addr, Requirements const& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int const sequencer[] = {0, (detail::add_allowance(addr, hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;
        }

        template <typename Requirements, std::size_t...Is>
        void add_allowance_left(runtime::HierarchyAddress const& addr, Requirements& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int const sequencer[] = {0, (detail::add_allowance_left(addr, hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;
        }

        template <typename Requirements, std::size_t...Is>
        void add_allowance_right(runtime::HierarchyAddress const& addr, Requirements& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int const sequencer[] = {0, (detail::add_allowance_right(addr, hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;
        }
    }

    template <typename Requirements>
    void
    add_allowance(runtime::HierarchyAddress const& addr, Requirements const& reqs)
    {
        return detail::add_allowance(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }

    template <typename Requirements>
    void
    add_allowance_left(runtime::HierarchyAddress const& addr, Requirements& reqs)
    {
        return detail::add_allowance_left(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }

    template <typename Requirements>
    void
    add_allowance_right(runtime::HierarchyAddress const& addr, Requirements& reqs)
    {
        return detail::add_allowance_right(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
