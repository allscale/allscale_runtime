
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_BASE_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_BASE_HPP

#include <allscale/treeture.hpp>
#include <allscale/this_work_item.hpp>

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
		work_item_impl_base() = default;

        work_item_impl_base(this_work_item::id id)
          : id_(std::move(id))
        {}

        virtual ~work_item_impl_base() = default;

        work_item_impl_base(work_item_impl_base const&) = delete;
		work_item_impl_base& operator=(work_item_impl_base const&) = delete;

        void update_rank(std::size_t rank)
        {
            id_.update_rank(rank);
        }

		void set_this_id(machine_config const& config);
		this_work_item::id const& id() const;
		virtual const char* name() const=0;

		virtual treeture<void> get_treeture() = 0;
		virtual bool valid()=0;

        virtual void on_ready(hpx::util::unique_function_nonser<void()> f)=0;

		virtual bool can_split() const=0;
		virtual hpx::future<std::size_t> process(executor_type& exec)=0;
		virtual hpx::future<std::size_t> split(bool sync)=0;

        virtual bool enqueue_remote() const=0;

        this_work_item::id id_;
    };
}}

HPX_TRAITS_NONINTRUSIVE_POLYMORPHIC(allscale::detail::work_item_impl_base);

#endif
