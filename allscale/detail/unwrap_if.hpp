
#ifndef ALLSCALE_DETAIL_UNWRAP_IF_HPP
#define ALLSCALE_DETAIL_UNWRAP_IF_HPP

#include <hpx/traits/future_traits.hpp>
#include <hpx/traits/is_future.hpp>

#include <type_traits>
#include <utility>
#include <iostream>
namespace allscale { namespace detail
{
        template <typename F>
        typename std::enable_if<
            hpx::traits::is_future<F>::value,
            typename hpx::traits::future_traits<F>::result_type
        >::type
        unwrap_if(F && f)
        {
            std::cout<<"checking if i should unwrap" << std::endl;
            return f.get();
        }

        template <typename F>
        typename std::enable_if<
            !hpx::traits::is_future<F>::value,
            typename std::remove_reference<F>::type &&
        >::type
        unwrap_if(F && f)
        {

            std::cout<<"checking if i should unwrap" << std::endl;
            return std::move(f);
        }
}}

#endif
