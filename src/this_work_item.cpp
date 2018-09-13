
#include <allscale/this_work_item.hpp>

#include <hpx/include/threads.hpp>

namespace allscale { namespace this_work_item {

    set::set(detail::work_item_impl_base& wi)
      : old(get())
    {
        hpx::this_thread::set_thread_data(reinterpret_cast<std::size_t>(&wi));
    }

    set::~set()
    {
        hpx::this_thread::set_thread_data(reinterpret_cast<std::size_t>(old));
    }

    detail::work_item_impl_base* get()
    {
//         if (hpx::this_thread::get_thread_data() == 0)
//         {
//             return nullptr
//         }
        return reinterpret_cast<detail::work_item_impl_base*>(hpx::this_thread::get_thread_data());
    }

}}
