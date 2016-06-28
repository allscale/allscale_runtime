
#ifndef ALLSCALE_WORK_ITEM_HPP
#define ALLSCALE_WORK_ITEM_HPP

#include <allscale/treeture.hpp>
#include <allscale/traits/is_treeture.hpp>
#include <allscale/traits/treeture_traits.hpp>

#include <hpx/dataflow.hpp>
#include <hpx/include/serialization.hpp>

#include <utility>

namespace allscale
{
    namespace detail
    {
        template <typename F>
        typename std::enable_if<
            hpx::traits::is_future<F>::value,
            typename hpx::traits::future_traits<F>::result_type
        >::type
        unwrap_if(F && f)
        {
            return f.get();
        }

        template <typename F>
        typename std::enable_if<
            !hpx::traits::is_future<F>::value,
            typename std::remove_reference<F>::type &&
        >::type
        unwrap_if(F && f)
        {
            return std::move(f);
        }

        template <typename F>
        typename std::enable_if<
            traits::is_treeture<F>::value,
            typename traits::treeture_traits<F>::future_type
        >::type
        futurize_if(F && f)
        {
            return f.get_future();
        }

        template <typename F>
        typename std::enable_if<
            !traits::is_treeture<F>::value,
            typename hpx::util::decay<F>::type
        >::type
        futurize_if(F && f)
        {
            return std::forward<F>(f);
        }
    }

//     template <typename WorkitemDescription, typename Closure>
    struct work_item
    {
        struct work_item_impl_base
          : boost::enable_shared_from_this<work_item_impl_base>
        {
            virtual ~work_item_impl_base()
            {}

            virtual void execute()=0;
            virtual bool valid()=0;

            template <typename Archive>
            void serialize(Archive & ar, unsigned)
            {
            }
            HPX_SERIALIZATION_POLYMORPHIC_ABSTRACT(work_item_impl_base);
        };

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
            boost::shared_ptr<work_item_impl> shared_this()
            {
                return
                    boost::static_pointer_cast<work_item_impl>(
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

            bool valid()
            {
                return bool(this->shared_from_this());
            }

            void execute()
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
            boost::shared_ptr<work_item_impl> shared_this()
            {
                return
                    boost::static_pointer_cast<work_item_impl>(
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

            void execute()
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

        work_item()
        {}

        template <typename WorkItemDescription, typename Treeture, typename ...Ts>
        work_item(WorkItemDescription, Treeture tre, Ts&&... vs)
          : impl_(
                new work_item_impl<
                    WorkItemDescription,
                    hpx::util::tuple<
                        typename hpx::util::decay<
                            decltype(detail::futurize_if(std::forward<Ts>(vs)))
                        >::type...
                    >
                >(
                    std::move(tre), detail::futurize_if(std::forward<Ts>(vs))...
                )
            )
        {}

        work_item(work_item const& other)
          : impl_(other.impl_)
        {}

        work_item(work_item && other)
          : impl_(std::move(other.impl_))
        {}

        work_item &operator=(work_item const& other)
        {
            impl_ = other.impl_;
            return *this;
        }

        work_item &operator=(work_item && other)
        {
            impl_ = std::move(other.impl_);
            return *this;
        }

        bool valid() const
        {
            if(impl_)
                return impl_->valid();
            return false;
        }

        void execute()
        {
            HPX_ASSERT(valid());
            HPX_ASSERT(impl_->valid());
            impl_->execute();
//             impl_.reset();
        }

        template <typename Archive>
        void load(Archive & ar, unsigned)
        {
            ar & impl_;
            HPX_ASSERT(impl_->valid());
        }

        template <typename Archive>
        void save(Archive & ar, unsigned) const
        {
            HPX_ASSERT(impl_->valid());

            if(ar.is_future_awaiting())
            {
                impl_->save(ar, 0);
            }
            else
            {
                ar & impl_;
            }
            HPX_ASSERT(impl_->valid());
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()

        boost::shared_ptr<work_item_impl_base> impl_;
    };
}

#endif
