#ifndef ALLSCALE_DATA_ITEM_REQUIREMENT
#define ALLSCALE_DATA_ITEM_REQUIREMENT

#include <allscale/data_item_reference.hpp>
#include <allscale/access_mode.hpp>
#include <hpx/runtime/serialization/input_archive.hpp>
#include <hpx/runtime/serialization/output_archive.hpp>

namespace allscale{

    template<typename DataItemType>
    struct data_item_requirement {
    public:
        using ref_type = data_item_reference<DataItemType>;
        using region_type = typename DataItemType::region_type;
        using data_item_type = DataItemType;

        ref_type ref;
        region_type region;
        access_mode mode;

        data_item_requirement()
          : mode(access_mode::Invalid)
        {
        }

        data_item_requirement(
                const data_item_reference<DataItemType>& pref,
                const typename DataItemType::region_type& pregion,
                const access_mode& pmode)
          : ref(pref) , region(pregion) , mode(pmode)
        {}

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
           ar & ref;
           ar & region;
           ar & mode;
        }
    };


    template<typename DataItemType>
    data_item_requirement<DataItemType> createDataItemRequirement
    (   const data_item_reference<DataItemType>& ref,
        const typename DataItemType::region_type& region,
        access_mode mode
    )
    {
        //instance a data_item_requirement
        return { ref, region, mode };
    }

    namespace detail
    {

        template <std::size_t I, std::size_t J, typename Vector, typename Tuple1,
            typename Tuple2, typename Enable = void>
        struct merge_data_item_reqs_create;

        template <std::size_t I, std::size_t J, typename Vector, typename Tuple1,
            typename Tuple2>
        struct merge_data_item_reqs_create<I, J, Vector, Tuple1, Tuple2,
            typename std::enable_if<
                I == hpx::util::tuple_size<typename std::decay<Tuple2>::type>::value
            >::type
        >
        {
            typedef Tuple1 type;

            template <typename Tuple>
            static type call(Vector const&, Tuple&& tuple1, Tuple2 const& tuple2)
            {
                return std::forward<Tuple>(tuple1);
            }

        };

        template <std::size_t I, std::size_t J, typename Vector, typename Tuple1,
            typename Tuple2>
        struct merge_data_item_reqs_create<I, J, Vector, Tuple1, Tuple2,
            typename std::enable_if<
                I < hpx::util::tuple_size<typename std::decay<Tuple2>::type>::value
            >::type
        >
        {
            typedef
                typename merge_data_item_reqs_create<I + 1, J, Vector,
                    decltype(
                        hpx::util::tuple_cat(
                            std::declval<Tuple1>(),
                            hpx::util::make_tuple(hpx::util::get<I>(std::declval<Tuple2&&>()))
                        )
                    ),
                    Tuple2
                >::type
                type;

            template <typename Vector_, typename Tuple1_, typename Tuple2_>
            static type call(Vector_&& v, Tuple1_&& tuple1, Tuple2_&& tuple2)
            {
                auto new_ =
                    hpx::util::tuple_cat(
                        std::forward<Tuple1_>(tuple1),
                        hpx::util::make_tuple(hpx::util::get<I>(std::forward<Tuple2_>(tuple2)))
                    );
                typedef decltype(new_) new_type;
                return
                    merge_data_item_reqs_create<I+1, J, Vector_, new_type, Tuple2_>::call(
                        std::forward<Vector_>(v), std::move(new_),
                        std::forward<Tuple2_>(tuple2)
                    );
            }
        };

        template <std::size_t I, typename Vector, typename Tuple1,
            typename Tuple2>
        struct merge_data_item_reqs_create<I, I, Vector, Tuple1, Tuple2,
            typename std::enable_if<
                I < hpx::util::tuple_size<typename std::decay<Tuple2>::type>::value
            >::type
        >
        {
            typedef
                typename merge_data_item_reqs_create<I + 1, I, Vector,
                    decltype(
                        hpx::util::tuple_cat(
                            std::declval<Tuple1>(),
                            hpx::util::make_tuple(std::declval<Vector>())
                        )
                    ),
                    Tuple2
                >::type
                type;

            template <typename Vector_, typename Tuple1_, typename Tuple2_>
            static type call(Vector_&& v, Tuple1_&& tuple1, Tuple2_&& tuple2)
            {
                auto new_ =
                    hpx::util::tuple_cat(
                        std::forward<Tuple1_>(tuple1),
                        hpx::util::make_tuple(std::forward<Vector_>(v))
                    );
                typedef decltype(new_) new_type;
                return
                    merge_data_item_reqs_create<I+1, I, Vector_, new_type, Tuple2_>::call(
                        std::forward<Vector_>(v), std::move(new_),
                        std::forward<Tuple2_>(tuple2)
                    );
            }
        };

        // Helper to determine the Index if a element in a tuple. Idx is used
        // to introspect the Idx-th element in Tuple
        template <std::size_t Idx, typename Tuple, std::size_t Jdx, typename UTuple, typename Enable=void>
        struct merge_index;

        // In the case, where we traversed all elements, we "return" -1
        template <typename std::size_t Idx, typename Tuple, std::size_t Jdx, typename UTuple>
        struct merge_index<Idx, Tuple, Jdx, UTuple,
            typename std::enable_if<
                Idx == hpx::util::tuple_size<Tuple>::value ||
                Jdx == hpx::util::tuple_size<UTuple>::value
            >::type
        >
        {
            static constexpr std::size_t value = std::size_t(-1);
            static constexpr bool push_back = false;
        };

        template <typename std::size_t Idx, typename Tuple, std::size_t Jdx, typename UTuple>
        struct merge_index<Idx, Tuple, Jdx, UTuple,
            typename std::enable_if<
                Idx < hpx::util::tuple_size<Tuple>::value &&
                Jdx < hpx::util::tuple_size<UTuple>::value
            >::type
        >
        {
            typedef
                typename std::decay<
                    typename hpx::util::tuple_element<Idx, Tuple>::type
                >::type
                type;

            typedef
                typename std::decay<
                    typename hpx::util::tuple_element<Jdx, UTuple>::type
                >::type
                utype;

            typedef merge_index<Idx + 1, Tuple, Jdx, UTuple> next_index;

            // Check if we have a match
            static constexpr std::size_t value =
                std::is_same<utype, type>::value || std::is_same<std::vector<utype>, type>::value
                ? Idx : next_index::value;

            // Signal that we already have a vector and may just insert the
            // element
            static constexpr bool push_back = std::is_same<std::vector<utype>, type>::value
                ? true : std::is_same<utype, type>::value ? false : next_index::push_back;
        };

        // This overload is called whenever we iterated over all elements. In
        // which case we just return our result.
        template <std::size_t Idx, typename Res, typename Tuple>
        Res merge_data_item_reqs(Res res, Tuple&& dir_tuple,
            // Check to see if we reached the end
            typename std::enable_if<
                Idx == hpx::util::tuple_size<Tuple>::value, void*
            >::type = nullptr)
        {
            return res;
        }

        // This overload is called whenever there is an element in the
        // tuple with the same vector type.
        template <std::size_t Idx, typename Res, typename Tuple>
        auto merge_data_item_reqs(Res res, Tuple&& dir_tuple,
            // Check to see if we reached the end
            typename std::enable_if<
                Idx < hpx::util::tuple_size<Tuple>::value
            , void*>::type = nullptr,
            // merge_index returns the index or -1 if the type has not been in
            // the map before
            typename std::enable_if<
                merge_index<
                    0, Res, Idx, typename std::decay<Tuple>::type
                >::value != std::size_t(-1) &&
                merge_index<
                    0, Res, Idx, typename std::decay<Tuple>::type
                >::push_back
            , void*>::type = nullptr
        )
        {
            typedef
                merge_index<0, Res, Idx, typename std::decay<Tuple>::type>
                this_index;
            typedef typename this_index::utype elem_type;
            constexpr std::size_t jdx = this_index::value;

            auto &vec = hpx::util::get<jdx>(res);
            bool merged = false;
            auto &&orig_dir = hpx::util::get<Idx>(std::forward<Tuple>(dir_tuple));
            for (auto& dir: vec)
            {
                if (dir.mode == orig_dir.mode && dir.ref.id() == orig_dir.ref.id())
                {
                    merged = true;
                    dir.region = elem_type::region_type::merge(dir.region, orig_dir.region);
                    break;
                }
            }

            if (!merged)
                vec.push_back(std::move(orig_dir));

            return merge_data_item_reqs<Idx + 1>(std::move(res),
                std::forward<Tuple>(dir_tuple));
        }

        // This overload is called whenever there is an element in the
        // tuple with the same type already and a vector needs to be created.
        template <std::size_t Idx, typename Res, typename Tuple>
        auto merge_data_item_reqs(Res res, Tuple&& dir_tuple,
            // Check to see if we reached the end
            typename std::enable_if<
                Idx < hpx::util::tuple_size<Tuple>::value
            , void*>::type = nullptr,
            // merge_index returns the index or -1 if the type has not been in
            // the map before
            typename std::enable_if<
                merge_index<
                    0, Res, Idx, typename std::decay<Tuple>::type
                >::value != std::size_t(-1) &&
                !merge_index<
                    0, Res, Idx, typename std::decay<Tuple>::type
                >::push_back
            , void*>::type = nullptr
        )
        {
            typedef
                merge_index<0, Res, Idx, typename std::decay<Tuple>::type>
                this_index;
            typedef typename this_index::utype elem_type;
            constexpr std::size_t jdx = this_index::value;
            typedef std::vector<elem_type> vector;

            vector v;
            v.reserve(2);
            v.push_back(std::move(hpx::util::get<jdx>(res)));

            auto &&orig_dir = hpx::util::get<Idx>(std::forward<Tuple>(dir_tuple));
            if (v[0].mode == orig_dir.mode && v[0].ref.id() == orig_dir.ref.id())
            {
                v[0].region = elem_type::region_type::merge(v[0].region, orig_dir.region);
            }
            else
            {
                v.push_back(std::move(orig_dir));
            }

            // We recursively advance to the element in the tuple, and append
            // the current element to the result tuple
            return merge_data_item_reqs<Idx + 1>(
                merge_data_item_reqs_create<0, jdx, vector, hpx::util::tuple<>, Res>::call(
                    v, hpx::util::tuple<>(), std::move(res)),
                std::forward<Tuple>(dir_tuple));
        }

        // This overload is called whenever there has not been an element in the
        // tuple with the same type, yet.
        template <std::size_t Idx, typename Res, typename Tuple>
        auto merge_data_item_reqs(Res res, Tuple&& dir_tuple,
            // Check to see if we reached the end
            typename std::enable_if<
                Idx < hpx::util::tuple_size<Tuple>::value>::type* = nullptr,
            // merge_index returns the index or -1 if the type has not been in
            // the map before
            typename std::enable_if<
                merge_index<
                    0, Res, Idx, typename std::decay<Tuple>::type
                >::value == std::size_t(-1)>::type* = nullptr
        )
        {
            // We recursively advance to the element in the tuple, and append
            // the current element to the result tuple
            return merge_data_item_reqs<Idx + 1>(
                hpx::util::tuple_cat(
                    std::move(res),
                    hpx::util::make_tuple(hpx::util::get<Idx>(std::forward<Tuple>(dir_tuple)))
                ),
                std::forward<Tuple>(dir_tuple)
            );
        }

        template <typename DIRTuple>
        auto merge_data_item_reqs(DIRTuple&& dir_tuple)
        {
            return merge_data_item_reqs<0>(
                hpx::util::tuple<>(),
                std::forward<DIRTuple>(dir_tuple)
            );
        }
    }
}
#endif
