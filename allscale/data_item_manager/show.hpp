
#ifndef ALLSCALE_DATA_ITEM_MANAGER_SHOW_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_SHOW_HPP

#include <allscale/lease.hpp>
#include <allscale/api/core/data.h>
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_manager/index_service.hpp>
#include <allscale/data_item_manager/fragment.hpp>
#include <allscale/data_item_manager/data_item_view.hpp>
#include <allscale/data_item_manager/location_info.hpp>

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
        void show(Requirement const& req)
        {
            std::cout
                << req.ref.id() << " "
                << req.region << " "
                << req.allowance << " "
                << ((req.mode == access_mode::ReadWrite) ? "rw" : "ro") << "\n"
                ;
        }

        template <typename Requirement, typename RequirementAllocator>
        void show(std::vector<Requirement, RequirementAllocator> const& reqs)
        {
            for (auto& req: reqs)
            {
                show(req);
            }
        }

        template <typename Requirements, std::size_t...Is>
        void show(Requirements const& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int const sequencer[] = {0, (detail::show(hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;
        }
    }

    template <typename Requirements>
    void show(Requirements const& reqs)
    {
        return detail::show(reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
