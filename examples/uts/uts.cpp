//  Copyright (c) 2016 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BRG_RNG

#include <examples/uts/params.hpp>

#include <allscale/no_split.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item_description.hpp>

#include <hpx/hpx_init.hpp>

#include <boost/program_options.hpp>

params p;


typedef std::size_t int_type;
ALLSCALE_REGISTER_TREETURE_TYPE(int_type);

struct add_name
{
    static const char *name()
    {
        return "add";
    }
};

struct add_variant;
using add_work =
    allscale::work_item_description<
        int_type,
        add_name,
        allscale::do_serialization,
        allscale::no_split<int_type>,
        add_variant
    >;

struct add_variant
{
    static constexpr bool valid = true;
    using result_type = int_type;

    // Just perform the addition, no extra tasks are spawned
    template <typename Closure>
    static allscale::treeture<int_type> execute(Closure const& closure)
    {
        auto v = hpx::util::get<0>(closure);
        return
            allscale::treeture<int_type>{
                0//std::accumulate(v.begin(), v.end(), hpx::util::get<1>(closure))
            };
    }
};

struct traverse_name
{
    static const char* name()
    {
        return "traverse";
    }
};

struct traverse_process;
struct traverse_split;
using traverse_work =
    allscale::work_item_description<
        int_type,
        traverse_name,
        allscale::do_serialization,
        traverse_split,
        traverse_process
    >;

std::size_t traverse(node parent, std::vector<allscale::treeture<int_type>>& children)
{
    hpx::util::high_resolution_timer t;
    std::size_t height = parent.height;

    int num_children = parent.get_num_children(p);
    int child_type = parent.child_type(p);

    parent.num_children = num_children;

    if (num_children > 0)
    {
        int num = 0;
        for (int i = 0; i != num_children; ++i)
        {
            node child;
            child.type = child_type;
            child.height = height + 1;
            for(int j = 0; j < p.compute_granularity; ++j)
            {
                rng_spawn(parent.state.state, child.state.state, i);
            }
            num += traverse(child, children);
        }
        num_children += num;
    }

    return num_children + 1;
}

struct traverse_process
{
    static constexpr bool valid = true;
    using result_type = int_type;

    template <typename Closure>
    static allscale::treeture<int_type> execute(Closure const& closure)
    {
        std::vector<allscale::treeture<int_type>> children;
        auto res = traverse(hpx::util::get<0>(closure), children);
        return
            allscale::spawn<add_work>(std::move(children), res);
    }
};

struct traverse_split
{
    static constexpr bool valid = true;
    using result_type = int_type;

    template <typename Closure>
    static allscale::treeture<int_type> execute(Closure const& closure)
    {
        node parent = hpx::util::get<0>(closure);


        int num_children = parent.get_num_children(p);
        int child_type = parent.child_type(p);
        int height = parent.height;

        if (num_children > 0)
        {
            std::vector<allscale::treeture<int_type>> treetures;
            treetures.reserve(num_children);
            for (int i = 0; i != num_children; ++i)
            {
                node child;
                child.type = child_type;
                child.height = height + 1;

                for(int j = 0; j < p.compute_granularity; ++j)
                {
                    rng_spawn(parent.state.state, child.state.state, i);
                }

                treetures.push_back(
                    allscale::spawn<traverse_work>(child)
                );
            }

            return allscale::spawn<add_work>(std::move(treetures), 1 + num_children);
        }

        return allscale::treeture<int_type>{1};
    }
};


int hpx_main(boost::program_options::variables_map & vm)
{
    p = params(vm);
//     allscale::components::monitor_component_init();

    // start allscale scheduler ...
    allscale::scheduler::run(hpx::get_locality_id());

    if(hpx::get_locality_id() == 0)
    {
        hpx::util::high_resolution_timer timer;
        node root;
        root.init_root(p);
        std::size_t num_nodes = allscale::spawn<traverse_work>(root).get_result();
        double elapsed = timer.elapsed();
        std::cerr << "traveresed " << num_nodes << " in " << elapsed << " seconds\n";
        allscale::scheduler::stop();
    }

    return hpx::finalize();
}

int main(int argc, char** argv)
{
    boost::program_options::options_description desc = uts_params_desc();
    return hpx::init(desc, argc, argv);
}
