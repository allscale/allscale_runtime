
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_BASE_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_BASE_HPP

#include <hpx/include/serialization.hpp>

#include <memory>

namespace allscale { namespace detail {
    struct work_item_impl_base
      : std::enable_shared_from_this<work_item_impl_base>
    {
        virtual ~work_item_impl_base()
        {}

        virtual void execute(bool split)=0;
        virtual bool valid()=0;

        template <typename Archive>
        void serialize(Archive & ar, unsigned)
        {
        }
        HPX_SERIALIZATION_POLYMORPHIC_ABSTRACT(work_item_impl_base);
    };
}}

#endif
