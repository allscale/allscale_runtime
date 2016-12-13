
#ifndef ALLSCALE_DETAIL_UNWRAP_IF_HPP
#define ALLSCALE_DETAIL_UNWRAP_IF_HPP

#include <hpx/traits/future_traits.hpp>
#include <hpx/traits/is_future.hpp>

#include <type_traits>
#include <utility>

namespace allscale { namespace detail
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
}}

#endif
