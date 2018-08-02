#ifndef ALLSCALE_WORK_ITEM_HPP
#define ALLSCALE_WORK_ITEM_HPP

#include <allscale/detail/work_item_impl_base.hpp>
#include <allscale/detail/work_item_impl.hpp>
#include <allscale/detail/futurize_if.hpp>
#include <allscale/treeture.hpp>
#include <allscale/task_id.hpp>
#include <allscale/this_work_item.hpp>
#include <allscale/traits/is_treeture.hpp>
#include <allscale/traits/treeture_traits.hpp>

#include <hpx/include/serialization.hpp>

#include <utility>
#include <iostream>

namespace allscale {
    struct work_item {
        work_item() = default;

        explicit work_item(std::shared_ptr<detail::work_item_impl_base> impl)
          : impl_(std::move(impl))
        {}

        work_item(work_item const& other) = default;
        work_item(work_item && other) noexcept = default;
        work_item &operator=(work_item const& other) = default;
        work_item &operator=(work_item && other) noexcept = default;

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

        task_id const& id() const
        {
            HPX_ASSERT(impl_);
            return impl_->id();
        }

        const char* name() const
        {
            HPX_ASSERT(impl_);
            return impl_->name();
        }

        treeture<void> get_void_treeture() const
        {
            return impl_->get_void_treeture();
        }

        hpx::future<std::size_t> split(std::size_t this_id)
        {
            HPX_ASSERT(valid());
            HPX_ASSERT(impl_->valid());
            return impl_->split(this_id);
    //         impl_.reset();
        }

        void on_ready(hpx::util::unique_function_nonser<void()> f)
        {
            impl_->on_ready(std::move(f));
        }

        hpx::future<std::size_t> process(executor_type& exec, std::size_t this_id) {
            HPX_ASSERT(valid());
            HPX_ASSERT(impl_->valid());
            return impl_->process(exec, this_id);
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
        }

        template<typename Archive>
        void save(Archive & ar, unsigned) const
        {
            HPX_ASSERT(impl_->valid());

            ar & impl_;
            HPX_ASSERT(impl_->valid());
        }

        void reset()
        {
            impl_.reset();
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()

        std::shared_ptr<detail::work_item_impl_base> impl_;
	};
}

#endif
