
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


// a handle for loop iterations
class pfor_loop_handle {

    allscale::treeture<pfor_res_type> treeture;

public:

    pfor_loop_handle() : treeture(1) {}

    pfor_loop_handle(allscale::treeture<pfor_res_type>&& treeture)
        : treeture(std::move(treeture)) {}

    pfor_loop_handle(const pfor_loop_handle&) = delete;
    pfor_loop_handle(pfor_loop_handle&&) = default;

    ~pfor_loop_handle() {
        if (treeture.valid()) treeture.wait();
    }
    
    pfor_loop_handle& operator=(const pfor_loop_handle&) = delete;
    pfor_loop_handle& operator=(pfor_loop_handle&& other) {
        treeture = std::move(other.treeture);
    }

    const allscale::treeture<pfor_res_type>& getTreeture() const {
        return treeture;
    }

    allscale::task_reference to_task_reference() const {
        return treeture.to_task_reference();
    }

};


////////////////////////////////////////////////////////////////////////////////
// PFor Work Item:
//  - Result: std::int64_t
//  - Closure: [std::int64_t,std::int64_t,hpx::util::tuple<ExtraParams...>]
//  - SplitVariant: split range in half, compute halfs
//  - ProcessVariant: cumpute the given range


template<typename Body, typename ... ExtraParams>
struct pfor_split_variant;

template<typename Body, typename ... ExtraParams>
struct pfor_process_variant;

struct pfor_name
{
    static const char *name()
    {
        return "pfor";
    }
};

template<typename Body, typename ... ExtraParams>
using pfor_work =
    allscale::work_item_description<
        pfor_res_type,
        pfor_name,
        allscale::no_serialization,
        pfor_split_variant<Body,ExtraParams...>,
        pfor_process_variant<Body,ExtraParams...>
    >;

template<typename Body, typename ... ExtraParams>
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
        auto extra = hpx::util::get<2>(closure);

        // check whether there are iterations left
        if (begin >= end) return allscale::treeture<result_type>(1);

        // compute the middle
        auto mid = begin + (end - begin) / 2;

        // spawn two new sub-tasks
        return allscale::runtime::treeture_combine(
            allscale::spawn<pfor_work<Body,ExtraParams...>>(begin,mid,extra),
            allscale::spawn<pfor_work<Body,ExtraParams...>>(mid,end,extra),
            std::plus<result_type>()
        );
    }
};

template<typename Body, typename ... ExtraParams>
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
        auto extra = hpx::util::get<2>(closure);

        // get a body instance
        Body body;

        // do some computation
        for(auto i = begin; i<end; i++) {
            body(i,extra);
        }

        // done
        return allscale::treeture<result_type>(1);
    }
};


template<typename Body, typename ... ExtraParams>
pfor_loop_handle pfor(int a, int b, const ExtraParams& ... params) {
    return allscale::spawn_first<pfor_work<Body,ExtraParams...>>(a,b,hpx::util::make_tuple(params...));
}




////////////////////////////////////////////////////////////////////////////////
// PFor Work Item with fine-grained synchronization (simple)
//  - Result: std::int64_t
//  - Closure: [std::int64_t,std::int64_t,hpx::util::tuple<ExtraParams...>]
//  - SplitVariant: split range in half, compute halfs
//  - ProcessVariant: cumpute the given range


using loop_dependencies = hpx::util::tuple<
    allscale::task_reference,          // < left
    allscale::task_reference,          // < center
    allscale::task_reference           // < right
>;


template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_split_variant;

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_process_variant;

struct pfor_neighbor_sync_name
{
    static const char *name()
    {
        return "pfor_neighbor_sync";
    }
};

template<typename Body, typename ... ExtraParams>
using pfor_neighbor_sync_work =
    allscale::work_item_description<
        pfor_res_type,
        pfor_name,
        allscale::no_serialization,
        pfor_neighbor_sync_split_variant<Body,ExtraParams...>,
        pfor_neighbor_sync_process_variant<Body,ExtraParams...>
    >;

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_split_variant
{
    static constexpr bool valid = true;
    using result_type = pfor_res_type;


    // It spawns two new tasks, processing each half the iterations
    template <typename Closure>
    static allscale::treeture<result_type> execute(Closure const& closure)
    {
        // extract parameters
        auto begin = hpx::util::get<0>(closure);        
        auto end   = hpx::util::get<1>(closure);
        auto extra = hpx::util::get<2>(closure);

        // extract the dependencies
        auto deps  = hpx::util::get<3>(closure);

        // check whether there are iterations left
        if (begin >= end) return allscale::treeture<result_type>(1);

        // compute the middle
        auto mid = begin + (end - begin) / 2;

        // refine dependencies
        auto dl = hpx::util::get<0>(deps);
        auto dc = hpx::util::get<1>(deps);
        auto dr = hpx::util::get<2>(deps);

        // refine dependencies
        auto dlr = dl.get_right_child();
        auto dcl = dc.get_left_child();
        auto dcr = dc.get_right_child();
        auto drl = dr.get_left_child();

        // create the dependencies
        auto depsL = allscale::after(dlr,dcl,dcr    );
        auto depsR = allscale::after(    dcl,dcr,drl);

        // spawn two new sub-tasks
        return allscale::runtime::treeture_combine(
            allscale::spawn_after<pfor_work<Body,ExtraParams...>>( depsL, begin, mid, extra, hpx::util::make_tuple(dlr,dcl,dcr)),
            allscale::spawn_after<pfor_work<Body,ExtraParams...>>( depsR, mid,   end, extra, hpx::util::make_tuple(dcl,dcr,drl)),
            std::plus<result_type>()
        );
    }
};

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_process_variant
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
        // extract parameters
        auto begin = hpx::util::get<0>(closure);        
        auto end   = hpx::util::get<1>(closure);
        auto extra = hpx::util::get<2>(closure);

        // get a body instance
        Body body;

        // do some computation
        for(auto i = begin; i<end; i++) {
            body(i,extra);
        }

        // done
        return allscale::treeture<result_type>(1);
    }
};


template<typename Body, typename ... ExtraParams>
pfor_loop_handle pfor_neighbor_sync(const pfor_loop_handle& loop, int a, int b, const ExtraParams& ... params) {
    allscale::treeture<pfor_res_type> done(1);
    auto deps = allscale::after(done.to_task_reference(),loop.to_task_reference(),done.to_task_reference());
    return allscale::spawn_first_after<pfor_neighbor_sync_work<Body,ExtraParams...>>(
        deps,
        a,b,                                                    // the range
        hpx::util::make_tuple(params...),                       // the body parameters
        hpx::util::make_tuple(done.to_task_reference(),loop.to_task_reference(),done.to_task_reference())     // initial dependencies
    );
}





