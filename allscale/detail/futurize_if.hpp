
#ifndef ALLSCALE_DETAIL_FUTURIZE_IF_HPP
#define ALLSCALE_DETAIL_FUTURIZE_IF_HPP

#include <allscale/traits/is_treeture.hpp>
#include <allscale/traits/treeture_traits.hpp>

#include <type_traits>
#include <utility>

namespace allscale { namespace detail
{
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
}}

#endif
