
#ifndef ALLSCALE_TRAITS_IS_TREETURE_HPP
#define ALLSCALE_TRAITS_IS_TREETURE_HPP

#include <type_traits>

namespace allscale { namespace traits {
    template <typename T>
    struct is_treeture
      : std::false_type
    {};

    template <typename T>
    struct is_treeture<T const>
      : is_treeture<T>
    {};

    template <typename T>
    struct is_treeture<T volatile>
      : is_treeture<T>
    {};

    template <typename T>
    struct is_treeture<T &>
      : is_treeture<T>
    {};

    template <typename T>
    struct is_treeture<T &&>
      : is_treeture<T>
    {};
}}

#endif
