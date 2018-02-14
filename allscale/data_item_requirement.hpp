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

		// -- a utility to check whether a type is contained in a list of types --

		template<typename T, typename ... List>
		struct contains_type;

		template<typename T>
		struct contains_type<T> : public std::false_type {};

		template<typename T, typename First, typename ... Rest>
		struct contains_type<T,First,Rest...> : public std::integral_constant<bool,std::is_same<T,First>::value || contains_type<T,Rest...>::value> {};


		// -- a utility to obtain the index of a type in a tuple type --

		template<typename T, typename Tuple>
		struct index_of;

		template<typename T>
		struct index_of<T,hpx::util::tuple<>> : public std::integral_constant<std::size_t,std::size_t(-1)> {};

		template<typename T, typename First, typename ... Rest>
		struct index_of<T,hpx::util::tuple<First,Rest...>>
			: public std::integral_constant<std::size_t,(std::is_same<T,First>::value)?0:1+index_of<T,hpx::util::tuple<Rest...>>::value> {};


		// -- a simple wrapper converting a type T in a std::vector<T> --

		template<typename T>
		using vector_of_t = std::vector<T>;


		// -- a utility to prepent an type to a tuple type --

		template<typename Element, typename Tuple>
		struct prepent_tuple;

		template<typename T, typename ... Es>
		struct prepent_tuple<T,hpx::util::tuple<Es...>> {
			using type = hpx::util::tuple<T,Es...>;
		};

		template<typename Element, typename Tuple>
		using prepent_tuple_t = typename prepent_tuple<Element,Tuple>::type;


		// -- a utility to compute the return type of a data requirement merge operation --

		template<typename TupleType>
		struct merge_data_item_reqs_result;

		template<>
		struct merge_data_item_reqs_result<hpx::util::tuple<>> {
			using type = hpx::util::tuple<>;
		};

		template<typename First, typename ... Rest>
		struct merge_data_item_reqs_result<hpx::util::tuple<First,Rest...>> {
			using type = typename std::conditional<
				contains_type<First,Rest...>::value,
				typename merge_data_item_reqs_result<hpx::util::tuple<Rest...>>::type,
				prepent_tuple_t<vector_of_t<First>,typename merge_data_item_reqs_result<hpx::util::tuple<Rest...>>::type>
			>::type;
		};


		// -- a utility to sort data requirements into vectors of requirements --

		template<std::size_t pos>
		struct merge_data_item_reqs_inserter {

			template<typename Tuple, typename Res>
			static void insert(Tuple&& tuple, Res& res) {
				const auto& cur = hpx::util::get<pos-1>(tuple);
				using require_type = typename std::decay<decltype(cur)>::type;
				hpx::util::get<index_of<vector_of_t<require_type>,Res>::value>(res).push_back(std::move(cur));
				merge_data_item_reqs_inserter<pos-1>::insert(std::move(tuple),res);
			}
		};

		template<>
		struct merge_data_item_reqs_inserter<0> {
			template<typename Tuple, typename Res>
			static void insert(Tuple&&, Res&) {
				// nothing
			}
		};


		// -- aggregates requirements into vectors of same-typed requirements --

		template <typename DIRTuple>
		auto merge_data_item_reqs(DIRTuple&& dir_tuple)
		{
			// create result
			using result_t = typename merge_data_item_reqs_result<DIRTuple>::type;
			result_t res;

			// fill result
			merge_data_item_reqs_inserter<hpx::util::tuple_size<DIRTuple>::value>::insert(std::move(dir_tuple),res);

			// done
			return std::move(res);
		}

    }
}
#endif
