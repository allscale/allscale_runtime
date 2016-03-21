
#ifndef ALLSCALE_TRAITS_TREETURE_TRAITS_HPP
#define ALLSCALE_TRAITS_TREETURE_TRAITS_HPP

#include <allscale/traits/is_treeture.hpp>

namespace allscale { namespace traits {
    template <
        typename T,
        typename Enable = typename std::enable_if<traits::is_treeture<T>::value>::type
    >
    struct treeture_traits
    {
        using future_type = typename T::future_type;
    };

    template <typename T>
    struct treeture_traits<T const>
      : treeture_traits<T>
    {};

    template <typename T>
    struct treeture_traits<T volatile>
      : treeture_traits<T>
    {};

    template <typename T>
    struct treeture_traits<T &>
      : treeture_traits<T>
    {};

    template <typename T>
    struct treeture_traits<T &&>
      : treeture_traits<T>
    {};
}}

#endif
