#ifndef ALLSCALE_TRAITS_IS_REGION_HPP
#define ALLSCALE_TRAITS_IS_REGION_HPP

#include <type_traits>

namespace allscale{ namespace traits{

    template <typename T>
    struct is_region 
        : std::false_type
    {};

    template <typename T>
    struct is_region<T const>
        : is_region<T>
    {};

    template <typename T>
    struct is_region<T volatile>
        : is_region<T>
    {};


    template <typename T>
    struct is_region<T &>
        : is_region<T>
    {};

    template <typename T>
    struct is_region<T &&>
        : is_region<T>
    {};
}}























#endif
