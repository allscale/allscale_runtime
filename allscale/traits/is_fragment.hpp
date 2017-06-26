
#ifndef ALLSCALE_TRAITS_IS_FRAGMENT_HPP
#define ALLSCALE_TRAITS_IS_FRAGMENT_HPP

#include <type_traits>

namespace allscale { namespace traits {
    template <typename T>
    struct is_fragment
      : std::false_type
    {};

    template <typename T>
    struct is_fragment<T const>
      : is_fragment<T>
    {};

    template <typename T>
    struct is_fragment<T volatile>
      : is_fragment<T>
    {};

    template <typename T>
    struct is_fragment<T &>
      : is_fragment<T>
    {};

    template <typename T>
    struct is_fragment<T &&>
      : is_fragment<T>
    {};
}}

#endif
