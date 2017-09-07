
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_BASE_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_BASE_HPP

#include <allscale/treeture.hpp>
#include <allscale/this_work_item.hpp>

#include <hpx/include/serialization.hpp>
#include <hpx/util/unique_function.hpp>

#include <memory>

namespace allscale { namespace detail {
    struct work_item_impl_base
      : std::enable_shared_from_this<work_item_impl_base>
    {
		work_item_impl_base()
        {}

        work_item_impl_base(this_work_item::id const& id)
          : id_(id)
        {}

        virtual ~work_item_impl_base()
        {}

        work_item_impl_base(work_item_impl_base const&) = delete;
		work_item_impl_base& operator=(work_item_impl_base const&) = delete;

		void set_this_id(bool is_first);
		this_work_item::id const& id() const;
		virtual const char* name() const=0;

		virtual treeture<void> get_treeture() = 0;
		virtual bool valid()=0;

        virtual void on_ready(hpx::util::unique_function_nonser<void()> f)=0;

		virtual bool can_split() const=0;
		virtual void process()=0;
		virtual void split()=0;

//		virtual void requires()=0;

		//virtual std::vector<allscale::fragment> requires()=0;

        virtual bool enqueue_remote() const=0;

        this_work_item::id id_;
    };
}}

HPX_TRAITS_NONINTRUSIVE_POLYMORPHIC(allscale::detail::work_item_impl_base);

#endif
