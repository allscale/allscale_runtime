
#ifndef ALLSCALE_OPTIMIZER_HPP
#define ALLSCALE_OPTIMIZER_HPP

#include <allscale/get_num_localities.hpp>
#include <allscale/task_times.hpp>
#include <allscale/tuning_objective.hpp>
#include <allscale/tuner.hpp>

#include <allscale/components/internodeoptimizer.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/traits/is_bitwise_serializable.hpp>

#include <allscale/components/nmsimplex_bbincr.hpp>

#include <iosfwd>
#include <vector>

namespace allscale {
    struct optimizer_state
    {
        float load_;
        task_times task_times_;

        float avg_time_;
        unsigned long long energy_;
        std::uint64_t active_frequency_;
        std::size_t active_cores_per_node_;
        std::size_t cores_per_node_;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & load_;
            ar & task_times_;
            ar & avg_time_;
            ar & energy_;
            ar & active_frequency_;
            ar & active_cores_per_node_;
            ar & cores_per_node_;
        }
    };


//     struct optimizer_state
//     {
//         optimizer_state() : load(1.f), active_frequency(1000.f), cores_per_node(1)
//         {}
//
//         optimizer_state(float l,
//                         float avg_last_iterations_time,
//                         unsigned long long energy,
//                         float freq,
//                         std::size_t cores)
//           : load(l)
//           , avg_time(avg_last_iterations_time)
//           , energy(energy)
//           , active_frequency(freq)
//           , cores_per_node(cores)
//         {}
//
//         float load;
//         float avg_time;
//         unsigned long long energy;
//         float active_frequency;
//         std::size_t cores_per_node;
//
//         template <typename Archive>
//         void serialize(Archive& ar, unsigned)
//         {
//             ar & load;
//             ar & avg_time;
//             ar & active_frequency;
//             ar & cores_per_node;
//         }
//     };

    struct global_optimizer
    {
        using mutex_type = hpx::lcos::local::spinlock;
        mutex_type mtx_;

        global_optimizer();

        global_optimizer(global_optimizer&& other)
          : active_nodes_(std::move(other.active_nodes_))
          , active_frequency_(other.active_frequency_)
          , tuner_(std::move(other.tuner_))
          , active_(other.active_)
          , localities_(std::move(other.localities_))
          , f_resource_max(other.f_resource_max)
          , f_resource_leeway(other.f_resource_leeway)
          , o_ino(std::move(o_ino))
          // VV: Used by balance_ino_nmd
          , nmd_initialized(other.nmd_initialized)
          , nmd(other.nmd)
          , nodes_min(other.nodes_min)
          , nodes_max(other.nodes_max)
          , threads_min(other.threads_min)
          , threads_max(other.threads_max)
          , previous_num_nodes(other.previous_num_nodes)
          , use_lopt(other.use_lopt)
          , last_optimization_score(other.last_optimization_score)
        {
            objectives_scale[0] = other.objectives_scale[0];
            objectives_scale[1] = other.objectives_scale[1];
            objectives_scale[2] = other.objectives_scale[2];
        }

        bool active() const
        {
            return active_;
        }

        double get_optimization_score();

        hpx::future<void> balance(bool);
        hpx::future<void> balance_ino(const std::vector<std::size_t> &old_mapping);
        hpx::future<void> balance_ino_nmd(const std::vector<std::size_t> &old_mapping);
        hpx::future<void> decide_random_mapping(const std::vector<std::size_t> &old_mapping);
        
        void signal_objective_changed();

        bool may_rebalance();

        std::size_t u_balance_every;
        std::size_t u_steps_till_rebalance;

        void tune(std::vector<optimizer_state> const& state);
        int nmd_initialized;
        std::vector<bool> active_nodes_;
        std::uint64_t active_frequency_;

        using config = std::pair<std::size_t, float>;
        // Hill climbing data
        std::unique_ptr<tuner> tuner_;
        tuning_objective objective_;

        // local state information
        bool active_;

        std::vector<hpx::id_type> localities_;

        // VV: balance_ino and balance_global data
        float f_resource_max, f_resource_leeway;
        std::size_t previous_num_nodes;
        int nodes_min, nodes_max, threads_min, threads_max;

        components::internode_optimizer_t o_ino;

        components::NelderMead nmd;
        double last_optimization_score;
        double objectives_scale[3];
        bool use_lopt;
    };
}

#endif
