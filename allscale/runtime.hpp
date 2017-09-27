
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
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_manager.hpp>

#include <hpx/hpx_init.hpp>
#include <hpx/util/find_prefix.hpp>
#include <hpx/util/invoke_fused.hpp>
#include <hpx/util/unwrapped.hpp>
#include <hpx/lcos/local/dataflow.hpp>

#include <boost/program_options.hpp>

namespace allscale {
namespace runtime {

// Adapting names for compiler
using AccessMode = allscale::access_mode;

template <typename T>
using DataItemReference = allscale::data_item_reference<T>;
template <typename T>
using DataItemRequirement = allscale::data_item_requirement<T>;

template<typename DataItemType>
data_item_requirement<DataItemType> createDataItemRequirement
(   const data_item_reference<DataItemType>& ref,
    const typename DataItemType::region_type& region,
    const access_mode& mode
)
{
    return allscale::createDataItemRequirement(ref, region, mode);
}

using DataItemManager = allscale::data_item_manager;

inline int spawn_main(int(*main_work)(hpx::util::tuple<int, char**> const&), int argc, char** argv)
{
    auto clos = hpx::util::make_tuple(argc, argv);
    return main_work(clos);
}

inline int spawn_main(int(*main_work)(hpx::util::tuple<> const&), int argc, char** argv)
{
    auto clos = hpx::util::make_tuple();
    return main_work(clos);
}

inline int spawn_main(treeture<int>(*main_work)(hpx::util::tuple<int, char**> const&), int argc, char** argv)
{
    auto clos = hpx::util::make_tuple(argc, argv);
    return main_work(clos).get_result();
}

inline int spawn_main(treeture<int>(*main_work)(hpx::util::tuple<> const&), int argc, char** argv)
{
    auto clos = hpx::util::make_tuple();
    return main_work(clos).get_result();
}

/**
 * A wrapper for the main function of an applicaiton handling the startup and
 * shutdown procedure as well as lunching the first work item.
 */
template<typename MainWorkItem>
int allscale_main(boost::program_options::variables_map &)
{
    // include monitoring support
    auto mon = allscale::monitor::run(hpx::get_locality_id());
    // include resilience support
    auto resi = allscale::resilience::run(hpx::get_locality_id());
    // start allscale scheduler ...
    auto sched = allscale::scheduler::run(hpx::get_locality_id());

    // trigger first work item on first node
    int res = 0;
    if (hpx::get_locality_id() == 0)
    {
        static const char* argv[] = {"allscale"};
        int argc = 1;

        res = spawn_main(&MainWorkItem::process_variant::execute, argc, const_cast<char **>(argv));
        allscale::scheduler::stop();
        allscale::resilience::stop();
        allscale::monitor::stop();

        hpx::finalize();
    }

    // Force the optimizer to initialize the runtime...
    if (sched && mon && resi)
        // return result (only id == 0 will actually)
        return res;
    else return 1;
}

/**
 * A wrapper for the main function of an applicaiton handling the startup and
 * shutdown procedure as well as lunching the first work item.
 */
template<typename MainWorkItem>
int main_wrapper(int argc = 0, char **argv = nullptr) {
    typedef int(*hpx_main_type)(boost::program_options::variables_map &);
    hpx_main_type f = &allscale_main<MainWorkItem>;
    boost::program_options::options_description desc;
    hpx::util::set_hpx_prefix(HPX_PREFIX);
    allscale::scheduler::setup_resources(f, desc, argc, argv);
    return hpx::init();
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

template<typename A, typename R, typename F, typename ... Cs>
struct prec_operation {

    hpx::util::tuple<Cs...> closure;
    F impl;

    treeture<R> operator()(const A& a) {
        return (*this).operator()(dependencies(hpx::make_ready_future()),a);
    }

    treeture<R> operator()(dependencies const& d, const A& a) {
    	// TODO: could not get tuple_cat working, wrote own version
    	return (*impl)(d,prepend(a,closure));
    }

};

template<typename A, typename R, typename F, typename ... Cs>
prec_operation<A,R,F,Cs...> make_prec_operation(
        const hpx::util::tuple<Cs...>& closure, F impl) {
    return prec_operation<A,R,F,Cs...>{closure,impl};
}

// ---- a generic treeture combine operator ----
template<typename A, typename B, typename Op, typename R = std::result_of_t<Op(A,B)>>
allscale::treeture<R> treeture_combine(
    const dependencies& dep,
    allscale::treeture<A>&& a,
    allscale::treeture<B>&& b,
    Op&& op)
{
    allscale::treeture<R> res(
        hpx::dataflow(hpx::util::unwrapped(std::forward<Op>(op)),
        a.get_future(), b.get_future(), dep.dep_));

    res.set_children(std::move(a), std::move(b));

    return res;
}

template<typename A, typename B, typename Op, typename R = std::result_of_t<Op(A,B)>>
allscale::treeture<R> treeture_combine(
    allscale::treeture<A>&& a,
    allscale::treeture<B>&& b,
    Op op)
{
    allscale::treeture<R> res(
        hpx::dataflow(hpx::util::unwrapped(std::forward<Op>(op)),
        a.get_future(), b.get_future()));

    res.set_children(std::move(a), std::move(b));

    return res;
}

allscale::treeture<void> treeture_combine(
    const dependencies& dep,
    allscale::treeture<void>&& a,
    allscale::treeture<void>&& b)
{
    allscale::treeture<void> res(hpx::when_all(dep.dep_,
        a.get_future(), b.get_future()));

    res.set_children(std::move(a), std::move(b));

    return res;
}

allscale::treeture<void> treeture_combine(allscale::treeture<void>&& a, allscale::treeture<void>&& b) {
    allscale::treeture<void> res(hpx::when_all(a.get_future(), b.get_future()));

    res.set_children(std::move(a), std::move(b));

    return res;
}


template<typename A, typename B>
allscale::treeture<void> treeture_parallel(const dependencies& dep, allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
    return treeture_combine(dep, std::move(a), std::move(b));
}

template<typename A, typename B>
allscale::treeture<void> treeture_parallel(allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
    return treeture_combine(std::move(a), std::move(b));
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
