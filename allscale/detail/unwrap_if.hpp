
#ifndef ALLSCALE_DETAIL_UNWRAP_IF_HPP
#define ALLSCALE_DETAIL_UNWRAP_IF_HPP

#include <allscale/traits/treeture_traits.hpp>

#include <hpx/traits/future_traits.hpp>
#include <hpx/traits/is_future.hpp>

#include <type_traits>
#include <utility>

namespace allscale { namespace detail
{
    template<typename F>
    typename std::enable_if<
        hpx::traits::is_future<F>::value &&
        !std::is_same<void, typename hpx::traits::future_traits<F>::type>::value,
        typename hpx::traits::future_traits<F>::result_type
    >::type
    unwrap_if(F && f)
    {
        return f.get();
    }

    template<typename F>
    typename std::enable_if<
		hpx::traits::is_future<F>::value
				&& std::is_same<void,
						typename hpx::traits::future_traits<F>::type>::value>::type
    unwrap_if(F && f) {
        f.get(); // propagate exceptions...
    }

template<typename F>
typename std::enable_if<!hpx::traits::is_future<F>::value,
		typename std::remove_reference<F>::type &&>::type unwrap_if(F && f) {
	return std::move(f);
}

template<typename Indices, typename Tuple, typename T,
		typename UnwrapResult = typename std::decay<
				decltype(unwrap_if(std::forward<T>(std::declval<T>())))>::type,
		typename Enable = void>
struct unwrap_tuple_impl;

template<typename T, typename UnwrapResult, std::size_t ...Is, typename ...Ts>
struct unwrap_tuple_impl<hpx::util::detail::pack_c<std::size_t, Is...>,
		hpx::util::tuple<Ts...>, T, UnwrapResult,
		typename std::enable_if<!std::is_same<void, UnwrapResult>::value>::type> {
	typedef hpx::util::tuple<typename std::decay<Ts>::type..., UnwrapResult> result_type;

	template<typename Tuple>
	static result_type call(Tuple&& tuple, T&& t) {
		return result_type(std::move(hpx::util::get<Is>(tuple))...,
				unwrap_if(std::forward<T>(t)));
	}
};

template<typename Indices, typename Tuple, typename T, typename UnwrapResult>
struct unwrap_tuple_impl<Indices, Tuple, T, UnwrapResult,
		typename std::enable_if<std::is_same<void, UnwrapResult>::value>::type> {
	typedef Tuple result_type;

	static result_type&& call(Tuple&& tuple, T&& t)
    {
		unwrap_if(std::forward<T>(t));
		return std::forward<Tuple>(tuple);
	}
};

template<typename Tuple, typename T>
auto unwrap_tuple(Tuple&& tuple, T&& t) {
	return unwrap_tuple_impl<
			typename hpx::util::detail::make_index_pack<
					hpx::util::tuple_size<Tuple>::value>::type, Tuple, T>::call(
			std::forward<Tuple>(tuple), std::forward<T>(t));
}

template<typename Tuple, typename Head, typename ... Ts>
auto unwrap_tuple(Tuple&& tuple, Head&& head, Ts&&... ts) {
	return unwrap_tuple(
			unwrap_tuple_impl<
					typename hpx::util::detail::make_index_pack<
							hpx::util::tuple_size<Tuple>::value>::type, Tuple,
					Head>::call(std::forward<Tuple>(tuple),
					std::forward<Head>(head)), std::forward<Ts>(ts)...);
}

inline auto unwrap_tuple(hpx::util::tuple<> t)
{
    return t;
}

template<typename T, typename SharedState>
inline void set_treeture(treeture<T>& t, SharedState& s) {
	t.set_value(std::move(*s->get_result()));
}

template<typename SharedState>
inline void set_treeture(treeture<void>& t, SharedState& s) {
// 	f.get(); // exception propagation
	t.set_value(hpx::util::unused_type{});
}
}}

#endif
