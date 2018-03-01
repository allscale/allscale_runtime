
#ifndef ALLSCALE_DATA_ITEM_MANAGER_ACQUIRE_RANK_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_ACQUIRE_RANK_HPP

#include <allscale/data_item_manager/data_item_store.hpp>
#include <allscale/data_item_manager/location_info.hpp>

#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/util/tuple.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        template <typename Requirement, typename LocationInfo>
        std::size_t acquire_rank(Requirement const& req, LocationInfo const& info)
        {
//             std::cout << "------------------------------\n";
//             std::cout << "Located " << req.ref.id() << " (" << req.region << ", " <<
//                 (req.mode == access_mode::ReadOnly? "ro" : "rw") << ") \n";
//             for (auto const& parts: info.regions)
//             {
//                 std::cout << "  located " << parts.second << " at " << parts.first << '\n';
//             }
//             std::cout << "------------------------------\n";

            // If it's a read only access, return -2
            if (req.mode == access_mode::ReadOnly) return std::size_t(-2);
            std::size_t rank(-1);
            HPX_ASSERT(!info.regions.empty());
            // If we have more than one part, we need to split
            if (info.regions.size() > 1)
            {
                return std::size_t(-1);
            }

            return info.regions.begin()->first;
        }

        template <typename Requirement, typename RequirementAllocator,
            typename LocationInfo, typename LocationInfoAllocator>
        std::size_t acquire_rank(std::vector<Requirement, RequirementAllocator> const& reqs,
            std::vector<LocationInfo, LocationInfoAllocator> const& infos)
        {
            HPX_ASSERT(reqs.size() == infos.size());
            std::size_t rank(-2);
            for (std::size_t i = 0; i != reqs.size(); ++i)
            {
                std::size_t cur_rank = detail::acquire_rank(reqs[i], infos[i]);
                if (cur_rank == std::size_t(-2)) continue;
                if (cur_rank == std::size_t(-1)) return cur_rank;

                if (cur_rank != rank)
                {
                    if (rank == std::size_t(-2))
                    {
                        // rank hasn't been set yet...
                        rank = cur_rank;
                    }
                    else
                    {
                        // We have a rank mismatch, should split
                        // TODO: Should we abort right here?
                        return std::size_t(-1);
                    }
                }
            }
            // If all match, all is good. return what we got
            return rank;
        }

        template <typename Requirements, typename LocationInfos, std::size_t...Is>
        std::size_t acquire_rank(Requirements const& reqs, LocationInfos const& infos,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            std::size_t ranks[] = {
                detail::acquire_rank(hpx::util::get<Is>(reqs), hpx::util::get<Is>(infos))...
            };
            std::size_t rank(-2);
            for (std::size_t cur_rank: ranks)
            {
                if (cur_rank == std::size_t(-2)) continue;
                if (cur_rank == std::size_t(-1)) return cur_rank;

                if (cur_rank != rank)
                {
                    if (rank == std::size_t(-2))
                    {
                        // rank hasn't been set yet...
                        rank = cur_rank;
                    }
                    else
                    {
                        // We have a rank mismatch, should split
                        // TODO: Should we abort right here?
                        return std::size_t(-1);
                    }
                }
            }
            // If all match, all is good. return what we got
            return rank;
        }
    }

    template <typename Requirements, typename LocationInfos>
    std::size_t acquire_rank(Requirements const& reqs, LocationInfos const& infos)
    {
        static_assert(
            hpx::util::tuple_size<Requirements>::type::value ==
            hpx::util::tuple_size<LocationInfos>::type::value,
            "requirements and location info sizes do not match");

        return detail::acquire_rank(reqs, infos,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }

    namespace detail
    {
        template <typename Requirement, typename LocationInfo>
        void print_location_info(Requirement const& req, LocationInfo const& info)
        {
            if (info.regions.size() > 1)
            {
                std::cerr << "Conflicting requirement " << typeid(req).name() << " with region: " << req.region << '\n';
                std::cerr << "Cannot resolve locations on " << hpx::get_locality_id() << ":\n";
                for (auto const& parts: info.regions)
                {
                    std::cerr << "  " << parts.second << " at " << parts.first << '\n';
                }
            }
        }

        template <typename Requirement, typename RequirementAllocator,
            typename LocationInfo, typename LocationInfoAllocator>
        void print_location_info(std::vector<Requirement, RequirementAllocator> const& reqs,
            std::vector<LocationInfo, LocationInfoAllocator> const& infos)
        {
            HPX_ASSERT(reqs.size() == infos.size());
            for (std::size_t i = 0; i != reqs.size(); ++i)
            {
                detail::print_location_info(reqs[i], infos[i]);
            }
        }

        template <typename Requirements, typename LocationInfos, std::size_t...Is>
        void print_location_info(Requirements const& reqs, LocationInfos const& infos,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int sequencer[] = {
                (detail::print_location_info(hpx::util::get<Is>(reqs), hpx::util::get<Is>(infos)), 0)...
            };
            (void)sequencer;
        }
    }

    template <typename Requirements, typename LocationInfos>
    void print_location_info(Requirements const& reqs, LocationInfos const& infos)
    {
        static_assert(
            hpx::util::tuple_size<Requirements>::type::value ==
            hpx::util::tuple_size<LocationInfos>::type::value,
            "requirements and location info sizes do not match");

        detail::print_location_info(reqs, infos,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
