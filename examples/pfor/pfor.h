
#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item_description.hpp>

#include <allscale/runtime.hpp>

#include <unistd.h>


typedef std::int64_t pfor_res_type;
ALLSCALE_REGISTER_TREETURE_TYPE(pfor_res_type);


////////////////////////////////////////////////////////////////////////////////
// PFor Work Item:
//  - Result: std::int64_t
//  - Closure: [std::int64_t,std::int64_t]
//  - SplitVariant: split range in half, compute halfs
//  - ProcessVariant: cumpute the given range


template<typename Body>
struct pfor_split_variant;

template<typename Body>
struct pfor_process_variant;

struct pfor_name
{
    static const char *name()
    {
        return "pfor";
    }
};

template<typename Body>
using pfor_work =
    allscale::work_item_description<
        pfor_res_type,
        pfor_name,
        allscale::no_serialization,
        pfor_split_variant<Body>,
        pfor_process_variant<Body>
    >;

template<typename Body>
struct pfor_split_variant
{
    static constexpr bool valid = true;
    using result_type = pfor_res_type;


    // It spawns two new tasks, processing each half the iterations
    template <typename Closure>
    static allscale::treeture<result_type> execute(Closure const& closure)
    {
        auto begin = hpx::util::get<0>(closure);        
        auto end   = hpx::util::get<1>(closure);

        // check whether there are iterations left
        if (begin >= end) return allscale::treeture<result_type>(1);

        // compute the middle
        auto mid = begin + (end - begin) / 2;

        // spawn two new sub-tasks
        return allscale::runtime::treeture_combine(
            allscale::spawn<pfor_work<Body>>(begin,mid),
            allscale::spawn<pfor_work<Body>>(mid,end),
            std::plus<result_type>()
        );
    }
};

template<typename Body>
struct pfor_process_variant
{
    static constexpr bool valid = true;
    using result_type = pfor_res_type;

    static const char* name()
    {
        return "pfor_process_variant";
    }

    // Execute for serial
    template <typename Closure>
    static allscale::treeture<result_type> execute(Closure const& closure)
    {
        auto begin = hpx::util::get<0>(closure);        
        auto end   = hpx::util::get<1>(closure);

        // get a body instance
        Body body;

        // do some computation
        for(auto i = begin; i<end; i++) {
            body(i);
        }

        // done
        return allscale::treeture<result_type>(1);
    }
};


template<typename Body>
void pfor(int a, int b) {
    allscale::spawn_first<pfor_work<Body>>(a,b).get_result();
}


