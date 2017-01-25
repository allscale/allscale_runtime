
//  Copyright (c) 2013 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BENCHMARKS_UTS_PARAMS_HPP
#define BENCHMARKS_UTS_PARAMS_HPP

#include <examples/uts/rng/rng.h>
#include <examples/uts/uts.hpp>

#include <hpx/runtime.hpp>

#include <cmath>
#include <iostream>

#include <boost/program_options.hpp>

struct params
{
    params()
    {}

    params(boost::program_options::variables_map & vm)
      : type(vm["tree-type"].as<node::tree_type>())
      , b_0(vm["root-branching-factor"].as<double>())
      , root_id(vm["root-seed"].as<int>())
      , shape_fn(vm["tree-shape"].as<node::geoshape>())
      , gen_mx(vm["tree-depth"].as<std::size_t>())
      , non_leaf_prob(vm["non-leaf-probability"].as<double>())
      , non_leaf_bf(vm["num-children"].as<int>())
      , shift_depth(vm["fraction-of-depth"].as<double>())
      , compute_granularity(vm["compute-granularity"].as<int>())
      , chunk_size(vm["chunk-size"].as<std::size_t>())
      , polling_interval(vm["interval"].as<int>())
      , verbose(vm["verbose"].as<int>())
      , debug(vm["debug"].as<int>())
    {}

    void print(const char * name) const
    {
        if(verbose == 0) return;

        std::cout
            << "UTS - Unbalanced Tree Search 0.1 (HPX " << name << ")\n"
            << "Tree type: " << static_cast<int>(type) << " (" << type << ")\n"
            << "Tree shape parameters:\n"
            << "  root branching factor b_0 = " << b_0 << ", root seed = " << root_id << "\n";

        if(type == node::GEO || type == node::HYBRID)
        {
            std::cout
                << "  GEO parameters: "
                << "gen_mx = " << gen_mx
                << ", shape function = " << static_cast<int>(shape_fn) << " (" << shape_fn << ")\n";
        }

        if(type == node::BIN || type == node::HYBRID)
        {
            double q = non_leaf_prob;
            int m = non_leaf_bf;
            double es = (1.0 / (1.0 - q * m));

            std::cout
                << "  BIN parameters: "
                << "q = " << q << ", m = " << m << ", E(n) = " << q * m << ", E(s) = " << es << "\n";
        }

        if(type == node::HYBRID)
        {
            std::cout
                << "  HYBRID: GEO from root to depth " << std::ceil(shift_depth * gen_mx) << ", then BIN\n";
        }

        if(type == node::BALANCED)
        {
            std::cout
                << "  BALANCED parameters: gen_mx = " << gen_mx << "\n"
                << "        Expected size: " << (std::pow(b_0, gen_mx + 1) - 1.0)/(b_0 - 1.0) << " nodes, "
                << std::pow(b_0, gen_mx) << " leaves\n";
        }

        std::size_t num_threads = hpx::get_num_worker_threads();

        // random number generator
        char strBuf[1024];
        rng_showtype(strBuf, 0);

        hpx::lcos::future<boost::uint32_t> locs = hpx::get_num_localities();
        std::cout
            << "Random number generator: " << strBuf << "\n"
            << "Compute granularity: " << compute_granularity << "\n"
            << "Execution strategy: "
            << "Parallel search using " << locs.get() << " localities "
            << "with a total of " << num_threads << " threads\n"
            << "Load balance by " << name << ", chunk size = " << chunk_size << " nodes\n"
            << "Polling Interval: " << polling_interval << "\n\n"
            ;
    }

    template <typename Archive>
    void serialize(Archive & ar, unsigned)
    {
        ar & type;
        ar & b_0;
        ar & root_id;
        ar & shape_fn;
        ar & gen_mx;
        ar & non_leaf_prob;
        ar & non_leaf_bf;
        ar & shift_depth;
        ar & compute_granularity;
        ar & chunk_size;
        ar & polling_interval;
        ar & verbose;
        ar & debug;
    }

    node::tree_type type;
    double b_0;
    int root_id;
    node::geoshape shape_fn;
    std::size_t gen_mx;
    double non_leaf_prob;
    int non_leaf_bf;
    double shift_depth;
    int compute_granularity;
    std::size_t chunk_size;
    int polling_interval;
    int verbose;
    int debug;
};

inline boost::program_options::options_description uts_params_desc()
{
    boost::program_options::options_description
        desc("Usage: " HPX_APPLICATION_STRING " [options]");

    desc.add_options()
        (
            "tree-type"
          , boost::program_options::value<node::tree_type>()->default_value(node::GEO)
          , "tree type (0: BIN, 1: GEO, 2: HYBRID, 3: BALANCED)"
        )
        (
            "root-branching-factor"
          , boost::program_options::value<double>()->default_value(4.0)
          , "root branching factor"
        )
        (
            "root-seed"
          , boost::program_options::value<int>()->default_value(0)
          , "root seed 0 <= r < 2^31"
        )
        (
            "tree-shape"
          , boost::program_options::value<node::geoshape>()->default_value(node::LINEAR)
          , "GEO: tree shape function"
        )
        (
            "tree-depth"
          , boost::program_options::value<std::size_t>()->default_value(6)
          , "GEO, BALANCED: tree depth"
        )
        (
            "non-leaf-probability"
          , boost::program_options::value<double>()->default_value(15.0 / 64.0)
          , "BIN: probability of non-leaf-node"
        )
        (
            "num-children"
          , boost::program_options::value<int>()->default_value(4)
          , "BIN: number of children for non-leaf node"
        )
        (
            "fraction-of-depth"
          , boost::program_options::value<double>()->default_value(0.5)
          , "HYBRID: fraction of depth for GEO -> BIN transition"
        )
        (
            "compute-granularity"
          , boost::program_options::value<int>()->default_value(1)
          , "compute granularity: number of rng_spawns per node"
        )
        (
            "chunk-size"
          , boost::program_options::value<std::size_t>()->default_value(20)
          , "chunksize for work sharing and work stealing"
        )
        (
            "interval"
          , boost::program_options::value<int>()->default_value(0)
          , "work stealing/sharing interval (stealing default: adaptive)"
        )
        (
            "overcommit-factor"
          , boost::program_options::value<float>()->default_value(1.0)
          , "Number of steal stacks to instantiantiate per locality: number_of_threads * overcommit-factor"
        )
        (
            "verbose"
          , boost::program_options::value<int>()->default_value(1)
          , "nonzero to set verbose output"
        )
        (
            "debug"
          , boost::program_options::value<int>()->default_value(0)
          , "debug"
        )
        ;

    return desc;
}

#endif
