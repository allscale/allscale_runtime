
#ifndef ALLSCALE_DATA_ITEM_MANAGER_LOCATE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_LOCATE_HPP

#include <allscale/config.hpp>
#include <allscale/get_num_localities.hpp>
#include <allscale/data_item_manager/data_item_store.hpp>
#include <allscale/data_item_manager/fragment.hpp>
#include <allscale/data_item_manager/shared_data.hpp>
#include <allscale/data_item_manager/location_info.hpp>

#include <hpx/util/annotated_function.hpp>

#include <hpx/include/actions.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/util/tuple.hpp>

#include <vector>
#include <boost/thread.hpp>
namespace allscale { namespace data_item_manager {
    namespace detail
    {
        enum class locate_state
        {
            init,
            up,
            down
        };

//         inline std::size_t get_parent_id(std::size_t this_id)
//         {
//             std::size_t level = 0;
//             std::size_t res = (this_id >> level) << level;
//             while (res > 0)
//             {
//                 ++level;
//                 std::size_t tmp_res = (this_id >> level) << level;
//                 if (tmp_res != res) return tmp_res;
//             }
//
//             return res;
//         }
//
//         template <locate_state state, typename Requirement>
//         hpx::future<location_info<typename Requirement::region_type>>
// #if defined(ALLSCALE_DEBUG_DIM)
//         locate(std::string name, Requirement req, std::size_t src_id);
// #else
//         locate(Requirement req, std::size_t src_id);
// #endif
//
//         template <locate_state state, typename Requirement>
//         struct locate_action
//           : hpx::actions::make_action<
//                 decltype(&locate<state, Requirement>),
//                 &locate<state, Requirement>,
//                 locate_action<state, Requirement>>::type
//         {};

        template <typename Requirement>
        location_info<typename Requirement::region_type>
        locate_root(Requirement req);

        template <typename Requirement>
        struct locate_root_action
          : hpx::actions::make_action<
                decltype(&locate_root<Requirement>),
                &locate_root<Requirement>,
                locate_root_action<Requirement>>::type
        {};

        template <typename Requirement>
        location_info<typename Requirement::region_type>
        locate_root(Requirement req)
        {
            using region_type = typename Requirement::region_type;
            using location_info_type = location_info<region_type>;
            using data_item_type = typename Requirement::data_item_type;
            using fragment_type = typename data_item_store<data_item_type>::data_item_type::fragment_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            location_info_type info;

            auto& item = data_item_store<data_item_type>::lookup(req.ref);
            boost::shared_lock<mutex_type> l(item.region_mtx);

            auto remainder = std::move(req.region);

            for (auto const& cached: item.location_cache.regions)
            {
                auto part = region_type::intersect(remainder, cached.second);
                // We got a hit!
                if (!part.empty())
                {
                    // Insert location information...
                    auto & info_part = info.regions[cached.first];

                    info_part = region_type::merge(info_part, part);

                    // Subtract what we got from what we requested
                    remainder = region_type::difference(remainder, part);

                    // If the remainder is empty, we got everything covered...
                    if (remainder.empty())
                    {
                        HPX_ASSERT(!info.regions.empty());
                        return info;
                    }
                }
            }
            return info;
        }


        template <locate_state state, typename Requirement>
        hpx::future<location_info<typename Requirement::region_type>>
#if defined(ALLSCALE_DEBUG_DIM)
        locate(std::string name, Requirement req, std::size_t src_id)
#else
        locate(Requirement req, std::size_t src_id)
#endif
        {
            using region_type = typename Requirement::region_type;
            using location_info_type = location_info<region_type>;
            using data_item_type = typename Requirement::data_item_type;
            using fragment_type = typename data_item_store<data_item_type>::data_item_type::fragment_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            HPX_ASSERT(!req.region.empty());

            std::size_t this_id = hpx::get_locality_id();

            location_info_type info;

            auto& item = data_item_store<data_item_type>::lookup(req.ref);

            region_type remainder = std::move(req.region);
            {
                boost::shared_lock<mutex_type> l(item.region_mtx);
                // FIXME: wait for migration and lock here.

                // Now try to locate ...

                // First check the intersection of our own region
                region_type part = region_type::intersect(remainder, item.owned_region);
                if (!part.empty())
                {
                    info.regions.insert(std::make_pair(this_id, part));
                    // Subtract what we got from what we requested
                    remainder = region_type::difference(remainder, part);

                    // If the remainder is empty, we got everything covered...
                    if (remainder.empty())
                    {
                        HPX_ASSERT(!info.regions.empty());
                        return hpx::make_ready_future(std::move(info));
                    }
                }

                // Lookup in our cache

                for (auto const& cached: item.location_cache.regions)
                {
                    part = region_type::intersect(remainder, cached.second);
                    // We got a hit!
                    if (!part.empty())
                    {
                        // Insert location information...
                        auto & info_part = info.regions[cached.first];

                        info_part = region_type::merge(info_part, part);

                        // Subtract what we got from what we requested
                        remainder = region_type::difference(remainder, part);

                        // If the remainder is empty, we got everything covered...
                        if (remainder.empty())
                        {
                            HPX_ASSERT(!info.regions.empty());
                            return hpx::make_ready_future(std::move(info));
                        }
                    }
                }

            }

            HPX_ASSERT(!remainder.empty());

            using locate_root_action_type = locate_root_action<Requirement>;
            hpx::future<location_info_type> remote_info =
                hpx::async<locate_root_action_type>(
                    hpx::naming::get_id_from_locality_id(0),
                    Requirement(req.ref, std::move(remainder), req.mode));

            return hpx::dataflow(hpx::util::annotated_function(
#if defined(ALLSCALE_DEBUG_DIM)
                [name = std::move(name), info = std::move(info), req, src_id, remainder = std::move(remainder)]
#else
                [info = std::move(info), req, src_id, remainder = std::move(remainder)]
#endif
                (hpx::future<location_info_type>&& remote_info_fut) mutable
                {
                    auto& item = data_item_store<data_item_type>::lookup(req.ref);
                    std::size_t this_id = hpx::get_locality_id();

                    // Merge infos
                    location_info_type remote_info = remote_info_fut.get();
                    if (remote_info.regions.size() > 0)
                    {
                        std::unique_lock<mutex_type> l(item.region_mtx);
                        for (auto const& remote_info: remote_info.regions)
                        {
                            // This marks the need to initialize the requested
                            // region
//                             if (src_id == this_id && remote_info.first == src_id)
//                             {
//                                 remainder = region_type::intersect(remainder, remote_info.second);
//                             }
//                             else
                            {
                                // update cache.
                                auto & cached_part = item.location_cache.regions[remote_info.first];
                                cached_part = region_type::merge(cached_part, remote_info.second);

                                remainder = region_type::difference(remainder, remote_info.second);

                                // Insert location information...
                                auto & part = info.regions[remote_info.first];
                                part = region_type::merge(part, remote_info.second);
                            }
                        }
                        remainder = region_type::difference(remainder, item.owned_region);
                    }

#if defined(ALLSCALE_DEBUG_DIM)
                    if (!remainder.empty())
                    {
                        HPX_ASSERT(locate_state::init == state);
//                         auto & part = info.regions[this_id];
//                         part = region_type::merge(part, remainder);
//                         std::unique_lock<mutex_type> l(item.region_mtx);
//                         // merge with our own region
//                         item.owned_region =
//                             region_type::merge(item.owned_region, remainder);
                        std::stringstream filename;
                        filename << "data_item." << this_id << ".log";
                        std::ofstream os(filename.str(), std::ios_base::app);
                        os
                            << "create("
                            << (req.mode == access_mode::ReadOnly? "ro" : "rw")
                            << "), "
                            << name
                            << ", "
                            << std::hex << req.ref.id().get_lsb() << std::dec
                            << ": "
                            << remainder
//                             << region_type::intersect(fragment(item).getTotalSize(), remainder)
                            << '\n';
                        os.close();
                    }
#endif
                    return std::move(info);
                }, "allscale::data_item_manager::locate::remote_cont"), std::move(remote_info)
            );
        }

        template <locate_state state, typename Requirement, typename Allocator>
        std::vector<hpx::future<location_info<typename Requirement::region_type>>>
#if defined(ALLSCALE_DEBUG_DIM)
        locate(std::string name, std::vector<Requirement, Allocator> const& reqs, std::size_t src_id)
#else
        locate(std::vector<Requirement, Allocator> const& reqs, std::size_t src_id)
#endif
        {
            static_assert(state == locate_state::init, "Wrong state");
            std::vector<hpx::future<location_info<typename Requirement::region_type>>> res;
            res.reserve(reqs.size());
            for (auto const& req: reqs)
            {
#if defined(ALLSCALE_DEBUG_DIM)
                res.push_back(locate<state>(name, req, src_id));
#else
                res.push_back(locate<state>(req, src_id));
#endif
            }
            return res;
        }

#if defined(ALLSCALE_DEBUG_DIM)
        template <locate_state state, typename Requirements, std::size_t...Is>
        auto locate(std::string name, Requirements const& reqs, hpx::util::detail::pack_c<std::size_t, Is...>)
         -> hpx::util::tuple<decltype(locate<state>(name, hpx::util::get<Is>(reqs), std::declval<std::size_t>()))...>
        {
            static_assert(state == locate_state::init, "Wrong state");
            std::size_t src_id = hpx::get_locality_id();
            return hpx::util::make_tuple(locate<state>(name, hpx::util::get<Is>(reqs), src_id)...);
        }
#else
        template <locate_state state, typename Requirements, std::size_t...Is>
        auto locate(Requirements const& reqs, hpx::util::detail::pack_c<std::size_t, Is...>)
         -> hpx::util::tuple<decltype(locate<state>(hpx::util::get<Is>(reqs), std::declval<std::size_t>()))...>
        {
            static_assert(state == locate_state::init, "Wrong state");
            std::size_t src_id = hpx::get_locality_id();
            return hpx::util::make_tuple(locate<state>(hpx::util::get<Is>(reqs), src_id)...);
        }
#endif
    }

#if defined(ALLSCALE_DEBUG_DIM)
    template <typename Requirements>
    auto locate(std::string name, Requirements const& reqs)
     -> decltype(detail::locate<detail::locate_state::init>(name, reqs,
            std::declval<typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type>()))
    {
        return detail::locate<detail::locate_state::init>(name, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
#else
    template <typename Requirements>
    auto locate(Requirements const& reqs)
     -> decltype(detail::locate<detail::locate_state::init>(reqs,
            std::declval<typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type>()))
    {
        return detail::locate<detail::locate_state::init>(reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
#endif

#if defined(ALLSCALE_DEBUG_DIM)
    namespace detail
    {
        template <typename Requirement>
        void log_req(std::string const& name, Requirement const& req)
        {
            using region_type = typename Requirement::region_type;
            using location_info_type = location_info<region_type>;
            using data_item_type = typename Requirement::data_item_type;
            using fragment_type = typename data_item_store<data_item_type>::data_item_type::fragment_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            auto& item = data_item_store<data_item_type>::lookup(req.ref);
            std::stringstream filename;
            filename << "data_item." << hpx::get_locality_id() << ".log";
            std::ofstream os(filename.str(), std::ios_base::app);
            os
                << "require("
                << (req.mode == access_mode::ReadOnly? "ro" : "rw")
                << "), "
                << name
                << ", "
                << std::hex << req.ref.id().get_lsb() << std::dec
                << ": "
                << req.region << '\n';
//                 << region_type::intersect(fragment(req.ref, item).getTotalSize(), req.region) << '\n';
            os.close();
        }

        template <typename Requirement, typename Allocator>
        void log_req(std::string const& name, std::vector<Requirement, Allocator> const& reqs)
        {
            for (std::size_t i = 0; i != reqs.size(); ++i)
            {
                detail::log_req(name, reqs[i]);
            }
        }

        template <typename Requirements, std::size_t...Is>
        void log_req(std::string const& name, Requirements const& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            int sequencer[] = {
                (detail::log_req(name, hpx::util::get<Is>(reqs)), 0)...
            };
            (void)sequencer;
        }
    }

    template <typename Requirements>
    void log_req(std::string const& name, Requirements const& reqs)
    {
        detail::log_req(name, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
#endif
}}

#endif
