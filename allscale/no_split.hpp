
#ifndef ALLSCALE_NO_SPLIT_HPP
#define ALLSCALE_NO_SPLIT_HPP

#include <allscale/treeture.hpp>

namespace allscale
{
    template <typename T>
    struct no_split
    {
        static constexpr bool valid = false;
        using result_type = T;

        template <typename Closure>
        static allscale::treeture<T> execute(Closure const& closure);
    };
}

#endif
