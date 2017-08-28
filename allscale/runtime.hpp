
#ifndef ALLSCALE_RUNTIME_HPP
#define ALLSCALE_RUNTIME_HPP

/**
 * The header defining the interface to be utilzed by AllScale runtime applications.
 * In particular, this is the interface utilized by the compiler when generating applications
 * targeting the AllScale runtime.
 */


#include <allscale/no_split.hpp>
#include <allscale/spawn.hpp>
#include <allscale/work_item_description.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/no_serialization.hpp>

#include <hpx/hpx_main.hpp>
#include <hpx/util/invoke_fused.hpp>
#include <hpx/util/unwrapped.hpp>
#include <hpx/lcos/local/dataflow.hpp>

namespace allscale {
namespace runtime {

/**
 * A wrapper for the main function of an applicaiton handling the startup and
 * shutdown procedure as well as lunching the first work item.
 */
template<typename MainWorkItem, typename ... Args>
int main_wrapper(const Args& ... args) {
    // include monitoring support
    allscale::monitor::run(hpx::get_locality_id());
    // include resilience support
    allscale::resilience::run(hpx::get_locality_id());
    // start allscale scheduler ...
    auto sched = allscale::scheduler::run(hpx::get_locality_id());

    // trigger first work item on first node
    int res = 0;
    if (hpx::get_locality_id() == 0) {

        res = allscale::spawn_first<MainWorkItem>(args...).get_result();
        allscale::scheduler::stop();
        allscale::resilience::stop();
        allscale::monitor::stop();
    }

    // Force the optimizer to initialize the runtime...
    if (sched)
        // return result (only id == 0 will actually)
        return res;
    else return 1;
}

dependencies after() {
	return dependencies(hpx::make_ready_future());
}

template<typename ... TaskRefs>
typename std::enable_if<(sizeof...(TaskRefs) > 0), dependencies>::type
after(TaskRefs&& ... task_refs) {
	return dependencies(hpx::when_all(task_refs.get_future()...));
}



// ---- a prec operation wrapper ----

template <typename Indices, typename Tuple>
struct packer;

template <std::size_t... Is, typename... Ts>
struct packer<
    hpx::util::detail::pack_c<std::size_t, Is...>,
    hpx::util::tuple<Ts...>>
{
    template <typename Tuple, typename T>
    hpx::util::tuple<typename std::decay<T>::type, Ts...> operator()(Tuple const& tuple, T&& t)
    {
        return hpx::util::make_tuple(std::forward<T>(t),
            hpx::util::get<Is>(tuple)...);
    }
};


template<typename A, typename ... Cs>
hpx::util::tuple<typename std::decay<A>::type,Cs...>
prepend(A&& a, hpx::util::tuple<Cs...> const& rest) {
    return packer<
        typename hpx::util::detail::make_index_pack<sizeof...(Cs)>::type,
        hpx::util::tuple<Cs...>>()(rest, std::forward<A>(a));
}

template<typename A, typename R, typename ... Cs>
struct prec_operation {

    hpx::util::tuple<Cs...> closure;

    treeture<R>(*impl)(const dependencies& d, const hpx::util::tuple<A,Cs...>&);

    treeture<R> operator()(const A& a) {
        return (*this).operator()(dependencies(hpx::make_ready_future()),a);
    }

    treeture<R> operator()(dependencies const& d, const A& a) {
    	// TODO: could not get tuple_cat working, wrote own version
    	return (*impl)(d,prepend(a,closure));
    }

};

template<typename A, typename R, typename ... Cs>
prec_operation<A,R,Cs...> make_prec_operation(
        const hpx::util::tuple<Cs...>& closure, treeture<R>(*impl)(const dependencies& d, const hpx::util::tuple<A,Cs...>&)) {
    return prec_operation<A,R,Cs...>{closure,impl};
}

// ---- a generic treeture combine operator ----

struct combine_name {
    static const char *name() {
        return "combine";
    }
};

template<typename Result>
struct combine_operation {

    static constexpr bool valid = true;
    using result_type = Result;

    // Just perform the addition, no extra tasks are spawned
    template <typename Closure>
    static allscale::treeture<result_type> execute(Closure const& closure) {
        return
            allscale::treeture<result_type>{
                hpx::util::invoke(hpx::util::get<2>(closure), hpx::util::get<0>(closure), hpx::util::get<1>(closure))
            };
    }
};

template<typename Result>
using combine =
    allscale::work_item_description<
        Result,
        combine_name,
        allscale::no_serialization,
        allscale::no_split<Result>,
        combine_operation<Result>
    >;

template<typename A, typename B, typename Op, typename R = std::result_of_t<Op(A,B)>>
allscale::treeture<R> treeture_combine(const dependencies& dep, allscale::treeture<A>&& a, allscale::treeture<B>&& b, Op&& op) {
    return allscale::treeture<R>(
        hpx::dataflow(hpx::util::unwrapped(std::forward<Op>(op)), a.get_future(), b.get_future(), dep.dep_));
}

template<typename A, typename B, typename Op, typename R = std::result_of_t<Op(A,B)>>
allscale::treeture<R> treeture_combine(allscale::treeture<A>&& a, allscale::treeture<B>&& b, Op op) {
    return allscale::treeture<R>(
        hpx::dataflow(hpx::util::unwrapped(std::forward<Op>(op)), a.get_future(), b.get_future()));
}

allscale::treeture<void> treeture_combine(const dependencies& dep, allscale::treeture<void>&& a, allscale::treeture<void>&& b) {
    return allscale::treeture<void>(hpx::when_all(dep.dep_, a.get_future(), b.get_future()));
}

allscale::treeture<void> treeture_combine(allscale::treeture<void>&& a, allscale::treeture<void>&& b) {
    return allscale::treeture<void>(hpx::when_all(a.get_future(), b.get_future()));
}


template<typename A, typename B>
allscale::treeture<void> treeture_parallel(const dependencies& dep, allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
	return allscale::treeture<void>(hpx::when_all(dep.dep_, a.get_future(), b.get_future()));
}

template<typename A, typename B>
allscale::treeture<void> treeture_parallel(allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
	return allscale::treeture<void>(hpx::when_all(a.get_future(), b.get_future()));
}


template<typename A, typename B>
allscale::treeture<void> treeture_sequential(const dependencies&, allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
	assert(false && "Not implemented!");
	return treeture_parallel(std::move(a),std::move(b));
}

template<typename A, typename B>
allscale::treeture<void> treeture_sequential(allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
	return treeture_sequential(dependencies{hpx::future<void>()}, std::move(a),std::move(b));
}


} // end namespace runtime
} // end namespace allscale

#endif
