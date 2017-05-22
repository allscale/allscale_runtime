
#ifndef ALLSCALE_TREETURE_FWD_HPP
#define ALLSCALE_TREETURE_FWD_HPP

#include <allscale/traits/is_treeture.hpp>
#include <type_traits>

namespace allscale {
    template <typename T>
    struct treeture;

    namespace detail {
        template <typename T>
        struct treeture_lco;

        template <typename T>
        struct treeture_task;
    }

    namespace traits
    {
        template <typename T>
        struct is_treeture<treeture<T>> : std::true_type {};
    }

    struct parent_arg {};
}

#endif
