
#ifndef ALLSCALE_OPTIMIZER_HPP
#define ALLSCALE_OPTIMIZER_HPP

#include <allscale/get_num_localities.hpp>
#include <allscale/task_times.hpp>
#include <allscale/tuning_objective.hpp>
#include <allscale/tuner.hpp>

#include <allscale/components/internodeoptimizer.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/traits/is_bitwise_serializable.hpp>

#include <iosfwd>
#include <vector>

namespace allscale {
    struct optimizer_state
    {
        float load_;
        task_times task_times_;

        float avg_time_;
        unsigned long long energy_;
        float active_frequency_;
        std::size_t cores_per_node_;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & load_;
            ar & task_times_;
            ar & avg_time_;
            ar & energy_;
            ar & active_frequency_;
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
        {}

        bool active() const
        {
            return active_;
        }

        hpx::future<void> balance(bool);
        hpx::future<void> balance_ino(const std::vector<std::size_t> &old_mapping);
        hpx::future<void> decide_random_mapping(const std::vector<std::size_t> &old_mapping);

        bool may_rebalance();

        std::size_t u_balance_every;
        std::size_t u_steps_till_rebalance;

        void tune(std::vector<optimizer_state> const& state);

        std::vector<bool> active_nodes_;
        float active_frequency_;

        using config = std::pair<std::size_t, float>;
        // Hill climbing data
        std::unique_ptr<tuner> tuner_;
        tuning_objective objective_;

        // local state information
        bool active_;

        std::vector<hpx::id_type> localities_;

        float f_resource_max, f_resource_leeway;

        components::internode_optimizer_t o_ino;
    };
}

#endif
