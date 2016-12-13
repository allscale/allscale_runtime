
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP

#include <allscale/treeture.hpp>
#include <allscale/detail/unwrap_if.hpp>
#include <allscale/detail/work_item_impl_base.hpp>

#include <hpx/include/dataflow.hpp>
#include <hpx/include/serialization.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/tuple.hpp>

#include <memory>
#include <utility>

namespace allscale { namespace detail {
    template <
        typename WorkItemDescription,
        typename Closure,
        bool Split = WorkItemDescription::split_variant::valid
    >
    struct work_item_impl;

    template <typename WorkItemDescription, typename Closure>
    struct work_item_impl<WorkItemDescription, Closure, true>
      : work_item_impl_base
    {
        std::shared_ptr<work_item_impl> shared_this()
        {
            return
                std::static_pointer_cast<work_item_impl>(
                    this->shared_from_this()
                );
        }

        using result_type = typename WorkItemDescription::result_type;
        using closure_type = Closure;

        work_item_impl()
        {}

        template <typename ...Ts>
        work_item_impl(treeture<result_type> tres, Ts&&... vs)
          : tres_(std::move(tres))
          , closure_(std::forward<Ts>(vs)...)
        {
            HPX_ASSERT(tres_.valid());
        }

        template <typename ...Ts>
        void execute_impl(bool split, Ts...vs)
        {
            treeture<result_type> tres = tres_;

            if(split)
            {
                WorkItemDescription::split_variant::execute(
                    hpx::util::make_tuple(
                        detail::unwrap_if(std::move(vs))...
                    )
                ).get_future().then(
                    [tres](hpx::future<result_type> ff) mutable
                    {
                        tres.set_value(ff.get());
                    }
                );
            }
            else
            {
                WorkItemDescription::process_variant::execute(
                    hpx::util::make_tuple(
                        detail::unwrap_if(std::move(vs))...
                    )
                ).get_future().then(
                    [tres](hpx::future<result_type> ff) mutable
                    {
                        tres.set_value(ff.get());
                    }
                );
            }
        }

        bool valid()
        {
            return bool(this->shared_from_this());
        }

        void execute(bool split)
        {
            execute(split,
                typename hpx::util::detail::make_index_pack<
                    hpx::util::tuple_size<closure_type>::type::value
                >::type()
            );
        }
        template <std::size_t... Is>
        void execute(bool split, hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            void (work_item_impl::*f)(bool,
                typename hpx::util::decay<decltype(hpx::util::get<Is>(closure_))>::type...
            ) = &work_item_impl::execute_impl;
            HPX_ASSERT(valid());
            hpx::dataflow(
                f
              , shared_this()
              , split
              , std::move(hpx::util::get<Is>(closure_))...
            );
        }

//             template <typename, typename> friend
//             struct ::hpx::serialization::detail::register_class_name;
//
//             static std::string hpx_serialization_get_name_impl()
//             {
//                 hpx::serialization::detail::register_class_name<
//                     work_item_impl>::instance.instantiate();
//                 return WorkItemDescription::split_variant::name();
//             }
//             virtual std::string hpx_serialization_get_name() const
//             {
//                 return work_item_impl::hpx_serialization_get_name_impl();
//             }

        template <typename Archive>
        void serialize(Archive &ar, unsigned)
        {
            ar & hpx::serialization::base_object<work_item_impl_base>(*this);
            ar & tres_;
            ar & closure_;
        }
        HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE(work_item_impl);

        treeture<result_type> tres_;
        closure_type closure_;
    };

    template <typename WorkItemDescription, typename Closure>
    struct work_item_impl<WorkItemDescription, Closure, false>
      : work_item_impl_base
    {
        std::shared_ptr<work_item_impl> shared_this()
        {
            return
                std::static_pointer_cast<work_item_impl>(
                    this->shared_from_this()
                );
        }

        using result_type = typename WorkItemDescription::result_type;
        using closure_type = Closure;

        work_item_impl()
        {}

        template <typename ...Ts>
        work_item_impl(treeture<result_type> tres, Ts&&... vs)
          : tres_(std::move(tres))
          , closure_(std::forward<Ts>(vs)...)
        {
            HPX_ASSERT(tres_.valid());
        }

        template <typename ...Ts>
        void execute_impl(Ts...vs)
        {
            treeture<result_type> tres = tres_;
            WorkItemDescription::process_variant::execute(
                hpx::util::make_tuple(
                    detail::unwrap_if(std::move(vs))...
                )
            ).get_future().then(
                [tres](hpx::future<result_type> ff) mutable
                {
                    tres.set_value(ff.get());
                }
            );
        }

        bool valid()
        {
            return tres_.valid() && bool(this->shared_from_this());
        }

        void execute(bool /*split unused*/)
        {
            execute(
                typename hpx::util::detail::make_index_pack<
                    hpx::util::tuple_size<closure_type>::type::value
                >::type()
            );
        }
        template <std::size_t... Is>
        void execute(hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            void (work_item_impl::*f)(
                typename hpx::util::decay<decltype(hpx::util::get<Is>(closure_))>::type...
            ) = &work_item_impl::execute_impl;
            HPX_ASSERT(valid());
            hpx::dataflow(
                f
              , shared_this()
              , std::move(hpx::util::get<Is>(closure_))...
            );
        }

//             template <typename, typename> friend
//             struct ::hpx::serialization::detail::register_class_name;
//
//             static std::string hpx_serialization_get_name_impl()
//             {
//                 hpx::serialization::detail::register_class_name<
//                     work_item_impl>::instance.instantiate();
//                 return WorkItemDescription::process_variant::name();
//             }
//             virtual std::string hpx_serialization_get_name() const
//             {
//                 return work_item_impl::hpx_serialization_get_name_impl();
//             }

        template <typename Archive>
        void serialize(Archive &ar, unsigned)
        {
            ar & hpx::serialization::base_object<work_item_impl_base>(*this);
            ar & tres_;
            ar & closure_;
        }
        HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE(work_item_impl);

        treeture<result_type> tres_;
        closure_type closure_;
    };
}}

#endif
