
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
#include <allscale/do_serialization.hpp>
#include <allscale/no_serialization.hpp>

#include <hpx/hpx_main.hpp>
#include <hpx/util/invoke_fused.hpp>

namespace allscale {
namespace runtime {

/**
 * A wrapper for the main function of an applicaiton handling the startup and
 * shutdown procedure as well as lunching the first work item.
 */
template<typename MainWorkItem, typename ... Args>
int main_wrapper(const Args& ... args) {
    // include monitoring support
    allscale::components::monitor_component_init();
    std::cout<<"wrapper started" << std::endl;
    // start allscale scheduler ...
    allscale::scheduler::run(hpx::get_locality_id());

    // trigger first work item on first node
    int res = 0;
    if (hpx::get_locality_id() == 0) {

        res = allscale::spawn<MainWorkItem>(args...).get_result();
        allscale::scheduler::stop();
    }

    // return result (only id == 0 will actually)
    return res;
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
allscale::treeture<R> treeture_combine(allscale::treeture<A>&& a, allscale::treeture<B>&& b, Op op) {
    return allscale::spawn<combine<R>>(std::move(a),std::move(b), op);
}

allscale::treeture<void> treeture_combine(allscale::treeture<void>&& a, allscale::treeture<void>&& b) {
    return allscale::treeture<void>(hpx::when_all(a.get_future(), b.get_future()));
}


} // end namespace runtime
} // end namespace allscale

#endif
