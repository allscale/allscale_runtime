
#ifndef ALLSCALE_DATA_ITEM_MANAGER_LOCATE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_LOCATE_HPP

#include <allscale/config.hpp>
#include <allscale/get_num_localities.hpp>
#include <allscale/data_item_manager/data_item_store.hpp>
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

        template <typename Requirement, typename Region>
        hpx::future<std::pair<Region, Region>>
        register_child(hpx::naming::gid_type ref, Region region, std::size_t child_rank);

        template <typename Requirement, typename Region>
        struct register_child_action
          : hpx::actions::make_direct_action<
                decltype(&register_child<Requirement, Region>),
                &register_child<Requirement, Region>,
                register_child_action<Requirement, Region>
            >::type
        {};

        template <typename Requirement, typename Region>
        hpx::future<std::pair<Region, Region>>
        register_child(hpx::naming::gid_type ref, Region region, std::size_t child_rank)
        {
            hpx::util::annotate_function("allscale::data_item_manager::register_child");
            using region_type = typename Requirement::region_type;
            using location_info_type = location_info<region_type>;
            using data_item_type = typename Requirement::data_item_type;
            using fragment_type = typename data_item_store<data_item_type>::data_item_type::fragment_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            std::size_t this_id = hpx::get_locality_id();
            auto& item = data_item_store<data_item_type>::lookup(ref);
            std::unique_lock<mutex_type> l(item.mtx);

            region = region_type::difference(region, item.owned_region);
            region = region_type::difference(region, item.right_region);
            region = region_type::difference(region, item.left_region);
            region = region_type::difference(region, item.parent_region);

            region_type new_parent;

            // Check if left child
            if (this_id * 2 + 1 == child_rank)
            {
                item.left_region = region_type::merge(item.left_region, region);
                new_parent = region_type::merge(item.owned_region, item.right_region);
            }

            // Check if right child
            if (this_id * 2 + 2 == child_rank)
            {
                item.right_region = region_type::merge(item.right_region, region);
                new_parent = region_type::merge(item.owned_region, item.left_region);
            }

            if (this_id == 0)
            {
                return hpx::make_ready_future(
                    std::make_pair(region, new_parent));
            }
            l.unlock();

            // FIXME: futurize, make resilient
            hpx::id_type target(
                hpx::naming::get_id_from_locality_id(
                    (this_id-1)/2
                )
            );

            // Register with our parent as well...
            return hpx::dataflow(hpx::launch::sync,
                hpx::util::annotated_function([region, new_parent = std::move(new_parent)](hpx::future<std::pair<Region, Region>> res_fut)
                {
                    auto res = res_fut.get();
                    return std::make_pair(
                        region_type::intersect(region, res.first),
                        region_type::merge(new_parent, res.second));
                }, "allscale::data_item_manager::register_child_cont"),
                hpx::async<register_child_action<Requirement, Region>>(
                    target, ref, region, this_id)
            );
        }

        template <locate_state state, typename Requirement>
        hpx::future<location_info<typename Requirement::region_type>>
#if defined(ALLSCALE_DEBUG_DIM)
        locate(std::string name, Requirement req);
#else
        locate(Requirement req);
#endif

        template <locate_state state, typename Requirement>
        struct locate_action
          : hpx::actions::make_direct_action<
                decltype(&locate<state, Requirement>),
                &locate<state, Requirement>,
                locate_action<state, Requirement>>::type
        {};

        template <locate_state state, typename Item, typename Region, typename Requirement, typename LocationInfo>
        hpx::future<LocationInfo> first_touch(Item& item, Region remainder, Requirement req, std::size_t this_id,
            LocationInfo info
#if defined(ALLSCALE_DEBUG_DIM)
          , std::string const& name
#endif
        )
        {
            hpx::util::annotate_function("allscale::data_item_manager::locate");
            using region_type = typename Requirement::region_type;
            using location_info_type = location_info<region_type>;
            using data_item_type = typename Requirement::data_item_type;
            using fragment_type = typename data_item_store<data_item_type>::data_item_type::fragment_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;
            // We have a first touch if we couldn't locate all parts and
            // the remainder is not part of our own region
            if (state == locate_state::init && !remainder.empty())
            {
                boost::shared_lock<mutex_type> l(item.mtx);
                remainder = region_type::difference(remainder, item.owned_region);
                if (!remainder.empty())
                {
                    l.unlock();
                    hpx::naming::gid_type id = req.ref.id();
                    // propagate information up.
                    return hpx::dataflow(hpx::launch::sync, hpx::util::annotated_function(
#if defined(ALLSCALE_DEBUG_DIM)
                        [this_id, req = std::move(req), &item, info = std::move(info), name]
#else
                        [this_id, req = std::move(req), &item, info = std::move(info)]
#endif
                        (hpx::future<std::pair<region_type, region_type>> fregistered) mutable
                        {
                            auto registered = fregistered.get();
                            std::unique_lock<mutex_type> l(item.mtx);

                            // Merge newly returned parent with our own
                            item.parent_region = region_type::merge(item.parent_region, registered.second);

                            // Account for over approximation...
                            auto remainder = region_type::difference(registered.first, item.parent_region);

                            // Merge with our own region
                            item.owned_region =
                                region_type::merge(item.owned_region, remainder);

                            info.regions.insert(std::make_pair(this_id, remainder));

                            // And finally, allocate the fragment if it wasn't there already.
                            if (item.fragment == nullptr)
                            {
                                typename data_item_type::shared_data_type shared_data_;
                                {
                                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                                    shared_data_ = shared_data(req.ref);
                                }
                                if (item.fragment == nullptr)
                                {
                                    item.fragment.reset(new fragment_type(std::move(shared_data_)));
                                }
                            }
#if defined(ALLSCALE_DEBUG_DIM)
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
                                << region_type::intersect(item.fragment->getTotalSize(), remainder)
                                << '\n';
                            os.close();
#endif
                            return info;
                        }, "allscale::data_item_manager::first_touch::register_child_cont"),
                        register_child<Requirement>(id, remainder, this_id)
                    );
                }
            }
            return hpx::make_ready_future(std::move(info));
        }

        template <locate_state state, typename Requirement>
        hpx::future<location_info<typename Requirement::region_type>>
#if defined(ALLSCALE_DEBUG_DIM)
        locate(std::string name, Requirement req)
#else
        locate(Requirement req)
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

            boost::shared_lock<mutex_type> l(item.mtx);
            // FIXME: wait for migration and lock here.

//             item.locate_access++;

            // Now try to locate ...
            region_type remainder = std::move(req.region);

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
//             hpx::util::high_resolution_timer timer;
//             hpx::util::high_resolution_timer intersect_timer;
//             hpx::util::high_resolution_timer merge_timer;
//             hpx::util::high_resolution_timer difference_timer;

            for (auto const& cached: item.location_cache.regions)
            {
//                 intersect_timer.restart();
                part = region_type::intersect(remainder, cached.second);
//                 item.intersect_time += intersect_timer.elapsed();
                // We got a hit!
                if (!part.empty())
                {
                    // Insert location information...
                    auto & info_part = info.regions[cached.first];

//                     merge_timer.restart();
                    info_part = region_type::merge(info_part, part);
//                     item.merge_time += merge_timer.elapsed();

//                     difference_timer.restart();
                    // Subtract what we got from what we requested
                    remainder = region_type::difference(remainder, part);
//                     item.difference_time += difference_timer.elapsed();

                    // If the remainder is empty, we got everything covered...
                    if (remainder.empty())
                    {
//                         item.cache_lookup_time += timer.elapsed();
                        HPX_ASSERT(!info.regions.empty());
                        return hpx::make_ready_future(std::move(info));
                    }
                }
            }
//             item.cache_lookup_time += timer.elapsed();
//             item.cache_miss++;

            // We couldn't satisfy the request with locally stored information...
            // now branch out to our parent and children...
            std::array<hpx::future<location_info_type>, 6> remote_infos;

            // Check parent
            part = region_type::intersect(remainder, item.parent_region);
            using locate_down_action_type = locate_action<locate_state::down, Requirement>;
            using locate_up_action_type = locate_action<locate_state::up, Requirement>;
            std::size_t num_remote = 0;
            if (!part.empty())
            {
                hpx::util::unlock_guard<boost::shared_lock<mutex_type>> ul(l);
                HPX_ASSERT(this_id != 0);
                hpx::id_type target(
                    hpx::naming::get_id_from_locality_id(
                        (this_id-1)/2
                    )
                );
                // FIXME: make resilient
                remote_infos[0] = hpx::async<locate_down_action_type>(
#if defined(ALLSCALE_DEBUG_DIM)
                    target, std::string(), Requirement(req.ref, part, req.mode)
#else
                    target, Requirement(req.ref, part, req.mode)
#endif
                );

                // Subtract what we got from what we requested
                remainder = region_type::difference(remainder, part);
                ++num_remote;
            }
            // Check left
            part = region_type::intersect(remainder, item.left_region);
            if (!part.empty())
            {
                hpx::util::unlock_guard<boost::shared_lock<mutex_type>> ul(l);
                hpx::id_type target(
                    hpx::naming::get_id_from_locality_id(
                        this_id * 2 + 1
                    )
                );
                // FIXME: make resilient
                remote_infos[1] = hpx::async<locate_up_action_type>(
#if defined(ALLSCALE_DEBUG_DIM)
                    target, std::string(), Requirement(req.ref, part, req.mode)
#else
                    target, Requirement(req.ref, part, req.mode)
#endif
                );

                // Subtract what we got from what we requested
                remainder = region_type::difference(remainder, part);
                ++num_remote;
            }
            // Check right...
            part = region_type::intersect(remainder, item.right_region);
            if (!part.empty())
            {
                hpx::util::unlock_guard<boost::shared_lock<mutex_type>> ul(l);
                hpx::id_type target(
                    hpx::naming::get_id_from_locality_id(
                        this_id * 2 + 2
                    )
                );
                // FIXME: make resilient
                remote_infos[2] = hpx::async<locate_up_action_type>(
#if defined(ALLSCALE_DEBUG_DIM)
                    target, std::string(), Requirement(req.ref, part, req.mode)
#else
                    target, Requirement(req.ref, part, req.mode)
#endif
                );

                // Subtract what we got from what we requested
                remainder = region_type::difference(remainder, part);
                ++num_remote;
            }
            l.unlock();

            if (!remainder.empty())
            {
                if (state != locate_state::up && this_id != 0)
                {
                    hpx::id_type target(
                        hpx::naming::get_id_from_locality_id(
                            (this_id-1)/2
                        )
                    );
                    // FIXME: make resilient
                    remote_infos[3] = hpx::async<locate_down_action_type>(
#if defined(ALLSCALE_DEBUG_DIM)
                        target, std::string(), Requirement(req.ref, remainder, req.mode)
#else
                        target, Requirement(req.ref, remainder, req.mode)
#endif
                    );
                    ++num_remote;
                }

//                 if (state != locate_state::down)
//                 {
//                     if (this_id * 2 + 1 < allscale::get_num_localities())
//                     {
//                         hpx::id_type target(
//                             hpx::naming::get_id_from_locality_id(
//                                 this_id * 2 + 1
//                             )
//                         );
//                         // FIXME: make resilient
//                         remote_infos[4] = hpx::async<locate_up_action_type>(
// #if defined(ALLSCALE_DEBUG_DIM)
//                             target, std::string(), Requirement(req.ref, remainder, req.mode)
// #else
//                             target, Requirement(req.ref, remainder, req.mode)
// #endif
//                         );
//                         ++num_remote;
//                     }
//                     if (this_id * 2 + 2 < allscale::get_num_localities())
//                     {
//                         hpx::id_type target(
//                             hpx::naming::get_id_from_locality_id(
//                                 this_id * 2 + 2
//                             )
//                         );
//                         // FIXME: make resilient
//                         remote_infos[5] = hpx::async<locate_up_action_type>(
// #if defined(ALLSCALE_DEBUG_DIM)
//                             target, std::string(), Requirement(req.ref, remainder, req.mode)
// #else
//                             target, Requirement(req.ref, remainder, req.mode)
// #endif
//                         );
//                         ++num_remote;
//                     }
//                 }
            }

            return hpx::dataflow(hpx::launch::sync, hpx::util::annotated_function(
#if defined(ALLSCALE_DEBUG_DIM)
                [name = std::move(name), info = std::move(info), req, remainder = std::move(remainder)]
#else
                [info = std::move(info), req, remainder = std::move(remainder), num_remote]
#endif
                (std::array<hpx::future<location_info_type>, 6>&& remote_infos) mutable
                {
                    auto& item = data_item_store<data_item_type>::lookup(req.ref);
                    std::size_t this_id = hpx::get_locality_id();

                    // Merge infos
                    if (num_remote > 0)
                    {
                        std::unique_lock<mutex_type> l(item.mtx);
                        for (auto& info_parts_fut: remote_infos)
                        {
                            if (info_parts_fut.valid())
                            {
                                auto info_parts = info_parts_fut.get();
                                for (auto const& remote_info: info_parts.regions)
                                {
                                    // update cache.
                                    auto & cached_part = item.location_cache.regions[remote_info.first];
                                    cached_part = region_type::merge(cached_part, remote_info.second);

                                    // Insert location information...
                                    auto & part = info.regions[remote_info.first];
                                    part = region_type::merge(part, remote_info.second);

                                    remainder = region_type::difference(remainder, remote_info.second);
                                }
                            }
                        }
                    }

                    return first_touch<state>(item, std::move(remainder), std::move(req), this_id, std::move(info)
#if defined(ALLSCALE_DEBUG_DIM)
                      , name
#endif
                    );
                }, "allscale::data_item_manager::locate::remote_cont"), std::move(remote_infos)
            );
        }

        template <locate_state state, typename Requirement, typename Allocator>
        std::vector<hpx::future<location_info<typename Requirement::region_type>>>
#if defined(ALLSCALE_DEBUG_DIM)
        locate(std::string name, std::vector<Requirement, Allocator> const& reqs)
#else
        locate(std::vector<Requirement, Allocator> const& reqs)
#endif
        {
            std::vector<hpx::future<location_info<typename Requirement::region_type>>> res;
            res.reserve(reqs.size());
            for (auto const& req: reqs)
            {
#if defined(ALLSCALE_DEBUG_DIM)
                res.push_back(locate<state>(name, req));
#else
                res.push_back(locate<state>(req));
#endif
            }
            return res;
        }

#if defined(ALLSCALE_DEBUG_DIM)
        template <locate_state state, typename Requirements, std::size_t...Is>
        auto locate(std::string name, Requirements const& reqs, hpx::util::detail::pack_c<std::size_t, Is...>)
         -> hpx::util::tuple<decltype(locate<state>(name, hpx::util::get<Is>(reqs)))...>
        {
            return hpx::util::make_tuple(locate<state>(name, hpx::util::get<Is>(reqs))...);
        }
#else
        template <locate_state state, typename Requirements, std::size_t...Is>
        auto locate(Requirements const& reqs, hpx::util::detail::pack_c<std::size_t, Is...>)
         -> hpx::util::tuple<decltype(locate<state>(hpx::util::get<Is>(reqs)))...>
        {
            return hpx::util::make_tuple(locate<state>(hpx::util::get<Is>(reqs))...);
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
            std::unique_lock<mutex_type> l(item.mtx);
            // Allocate the fragment if it wasn't there already.
            if (item.fragment == nullptr)
            {
                typename data_item_type::shared_data_type shared_data_;
                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    shared_data_ = shared_data(req.ref);
                }
                if (item.fragment == nullptr)
                {
                    item.fragment.reset(new fragment_type(std::move(shared_data_)));
                }
            }
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
                << region_type::intersect(item.fragment->getTotalSize(), req.region) << '\n';
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
