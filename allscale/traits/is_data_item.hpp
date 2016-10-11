
#ifndef ALLSCALE_TRAITS_IS_DATA_ITEM_HPP
#define ALLSCALE_TRAITS_IS_DATA_ITEM_HPP

#include <type_traits>

namespace allscale { namespace traits {
    template <typename T>
    struct is_data_item
      : std::false_type
    {};

    template <typename T>
    struct is_data_item<T const>
      : is_data_item<T>
    {};

    template <typename T>
    struct is_data_item<T volatile>
      : is_data_item<T>
    {};

    template <typename T>
    struct is_data_item<T &>
      : is_data_item<T>
    {};

    template <typename T>
    struct is_data_item<T &&>
      : is_data_item<T>
    {};
}}

#endif
