
#ifndef ALLSCALE_TRAITS_DATA_ITEM_TRAITS_HPP
#define ALLSCALE_TRAITS_DATA_ITEM_TRAITS_HPP

#include <allscale/traits/is_data_item.hpp>

namespace allscale { namespace traits {
    template <
        typename T,
        typename Enable = typename std::enable_if<traits::is_data_item<T>::value>::type
    >
    struct data_item_traits
    {
        using future_type = typename T::future_type;
    };

    template <typename T>
    struct data_item_traits<T const>
      : data_item_traits<T>
    {};

    template <typename T>
    struct data_item_traits<T volatile>
      : data_item_traits<T>
    {};

    template <typename T>
    struct data_item_traits<T &>
      : data_item_traits<T>
    {};

    template <typename T>
    struct data_item_traits<T &&>
      : data_item_traits<T>
    {};
}}

#endif
