
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_BASE_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_BASE_HPP

#include <allscale/treeture.hpp>
#include <allscale/task_id.hpp>
#include <allscale/hierarchy.hpp>
#include <allscale/data_item_manager/task_requirements.hpp>

#include <hpx/include/serialization.hpp>
#include <hpx/util/unique_function.hpp>
#include <hpx/runtime/threads/executors/pool_executor.hpp>

#include <memory>

namespace allscale {
    using executor_type = hpx::threads::executors::pool_executor;
}

namespace allscale { namespace detail {
    struct work_item_impl_base
      : std::enable_shared_from_this<work_item_impl_base>
    {
        using task_requirements =
            std::unique_ptr<data_item_manager::task_requirements_base>;

		work_item_impl_base() = default;

        work_item_impl_base(task_id const& id)
          : id_(std::move(id))
          , num_children(0)
        {}

        virtual ~work_item_impl_base() = default;

        work_item_impl_base(work_item_impl_base const&) = delete;
		work_item_impl_base& operator=(work_item_impl_base const&) = delete;

		task_id const& id() const { return id_; }
		virtual const char* name() const=0;

		virtual treeture<void> get_void_treeture() const = 0;
		virtual bool valid()=0;

        virtual void on_ready(hpx::util::unique_function_nonser<void()> f)=0;

		virtual bool can_split() const=0;
		virtual void process(executor_type& exec, task_requirements&&)=0;
		virtual void split(executor_type& exec, task_requirements&&)=0;

        virtual task_requirements get_task_requirements() const = 0;

        virtual bool enqueue_remote() const=0;

        task_id id_;
        std::uint8_t num_children;
    };
}}

HPX_TRAITS_NONINTRUSIVE_POLYMORPHIC(allscale::detail::work_item_impl_base);

#endif
