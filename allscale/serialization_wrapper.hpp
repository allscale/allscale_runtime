/*

#include <hpx/runtime/serialization/serialize.hpp>
#include <allscale/utils/serializer.h>

namespace hpx { namespace serialization
{
    template <typename T>
    typename std::enable_if<
        ::allscale::utils::is_serializable<T>::value &&
        !(std::is_integral<T>::value || std::is_floating_point<T>::value),
        output_archive&
    >::type
    serialize(output_archive & ar, T const & t, int)
    {
//         t.store(ar);
        return ar;
    }

    template <typename T>
    typename std::enable_if<
        ::allscale::utils::is_serializable<T>::value &&
        !(std::is_integral<T>::value || std::is_floating_point<T>::value),
        input_archive&
    >::type
    serialize(input_archive & ar, T & t, int)
    {
//         t = T::load(ar);
        return ar;
    }
}

}

*/
