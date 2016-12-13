
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

#include <hpx/hpx_main.hpp>

namespace allscale {
namespace runtime {

/**
 * A wrapper for the main function of an applicaiton handling the startup and
 * shutdown procedure as well as lunching the first work item.
 */
template<typename MainWorkItem, typename ... Args>
int main_wrapper(const Args& ... args) {

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

template<typename A, typename B, typename Op>
struct combine_operation {

    static constexpr bool valid = true;
    using result_type = std::result_of_t<Op(A,B)>;

    // Just perform the addition, no extra tasks are spawned
    template <typename Closure>
    static allscale::treeture<result_type> execute(Closure const& closure) {
        return
            allscale::treeture<result_type>{
                Op()(hpx::util::get<0>(closure),hpx::util::get<1>(closure))
            };
    }
};

template<typename A, typename B, typename Op>
using combine = 
    allscale::work_item_description<
        std::result_of_t<Op(A,B)>,
        combine_name,
        allscale::no_split<std::result_of_t<Op(A,B)>>,
        combine_operation<A,B,Op>
    >;


template<typename A, typename B, typename Op, typename R = std::result_of_t<Op(A,B)>>
allscale::treeture<R> treeture_combine(const allscale::treeture<A>&& a, const allscale::treeture<B>& b, const Op&) {
    return allscale::spawn<combine<A,B,Op>>(a,b);
}


} // end namespace runtime
} // end namespace allscale

#endif
