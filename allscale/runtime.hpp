
#ifndef ALLSCALE_RUNTIME_HPP
#define ALLSCALE_RUNTIME_HPP

/**
 * The header defining the interface to be utilzed by AllScale runtime applications.
 * In particular, this is the interface utilized by the compiler when generating applications
 * targeting the AllScale runtime.
 */


#include <allscale/work_item_description.hpp>
#include <allscale/spawn.hpp>

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

} // end namespace runtime
} // end namespace allscale

#endif
