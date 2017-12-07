#ifndef ALLSCALE_WORK_ITEM_HPP
#define ALLSCALE_WORK_ITEM_HPP

#include <allscale/detail/work_item_impl_base.hpp>
#include <allscale/detail/work_item_impl.hpp>
#include <allscale/detail/futurize_if.hpp>
#include <allscale/treeture.hpp>
#include <allscale/this_work_item.hpp>
#include <allscale/traits/is_treeture.hpp>
#include <allscale/traits/treeture_traits.hpp>

#include <hpx/include/serialization.hpp>

#include <utility>
#include <iostream>

namespace allscale {
    struct work_item {
        work_item()
          : is_first_(false)
        {}

        template<typename WorkItemDescription, typename Treeture, typename ...Ts>
        work_item(bool is_first, WorkItemDescription, Treeture tre, Ts&&... vs)
          : impl_(
                new detail::work_item_impl<
                    WorkItemDescription,
                    hpx::util::tuple<
                        typename hpx::util::decay<
                            decltype(detail::futurize_if(std::forward<Ts>(vs)))>::type...
                    >
                >(
                    std::move(tre),
                    detail::futurize_if(std::forward<Ts>(vs))...)
            ),
            is_first_(is_first)
        {
        }

        template<typename WorkItemDescription, typename Treeture, typename ...Ts>
        work_item(bool is_first, WorkItemDescription, hpx::shared_future<void> dep, Treeture tre, Ts&&... vs)
          : impl_(
                new detail::work_item_impl<
                    WorkItemDescription,
                    hpx::util::tuple<
                        typename hpx::util::decay<
                            decltype(detail::futurize_if(std::forward<Ts>(vs)))>::type...
                    >
                >(
                    std::move(dep), std::move(tre),
                    detail::futurize_if(std::forward<Ts>(vs))...)
            ),
            is_first_(is_first)
        {
        }

        void set_this_id(machine_config const& mconfig)
        {
            impl_->set_this_id(mconfig);
        }

        bool is_first()
        {
            return is_first_;
        }

        explicit work_item(std::shared_ptr<detail::work_item_impl_base> impl)
          : impl_(std::move(impl)), is_first_(false)
        {}

        work_item(work_item const& other)
          : impl_(other.impl_), is_first_(other.is_first_)
        {}

        work_item(work_item && other)
          : impl_(std::move(other.impl_)), is_first_(other.is_first_)
        {}

        work_item &operator=(work_item const& other)
        {
            impl_ = other.impl_;
            is_first_ = other.is_first_;
            return *this;
        }

        work_item &operator=(work_item && other)
        {
            impl_ = std::move(other.impl_);
            is_first_ = other.is_first_;
            return *this;
        }

        bool valid() const
        {
            if (impl_)
            return impl_->valid();
            return false;
        }

        bool can_split() const
        {
            HPX_ASSERT(impl_);
            return impl_->can_split();
        }

        this_work_item::id const& id() const
        {
            if (impl_)
                return impl_->id();
            return this_work_item::get_id();
        }

        const char* name() const
        {
            if (impl_)
            return impl_->name();
            return "";
        }

        treeture<void> get_treeture()
        {
            return impl_->get_treeture();
        }

        void split(executor_type& exec, bool sync)
        {
            HPX_ASSERT(valid());
            HPX_ASSERT(impl_->valid());
            impl_->split(exec, sync);
    //         impl_.reset();
        }

        void on_ready(hpx::util::unique_function_nonser<void()> f)
        {
            impl_->on_ready(std::move(f));
        }

        void process(executor_type& exec, bool sync) {
            HPX_ASSERT(valid());
            HPX_ASSERT(impl_->valid());
            impl_->process(exec, sync);
    //         impl_.reset();
        }

        bool enqueue_remote() const
        {
            return impl_->enqueue_remote();
        }

        template<typename Archive>
        void load(Archive & ar, unsigned)
        {
            ar & impl_;
            HPX_ASSERT(impl_->valid());
            ar & is_first_;
        }

        template<typename Archive>
        void save(Archive & ar, unsigned) const
        {
            HPX_ASSERT(impl_->valid());

//             if (ar.is_preprocessing())
//             {
//                 impl_->preprocess(ar);
//             }
//             else
            {
                ar & impl_;
            }
            HPX_ASSERT(impl_->valid());
            ar & is_first_;
        }

        void reset()
        {
            impl_.reset();
            is_first_ = false;
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()

        std::shared_ptr<detail::work_item_impl_base> impl_;
        bool is_first_;
	};
}

#endif
