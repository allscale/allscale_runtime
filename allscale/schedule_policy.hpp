
// Original code by Herbert Jordan, adapted by Thomas Heller


#ifndef ALLSCALE_SCHEDULE_POLICY_HPP
#define ALLSCALE_SCHEDULE_POLICY_HPP

#include "allscale/utils/serializer.h"
#include <allscale/task_id.hpp>
#include <allscale/hierarchy.hpp>
#include <allscale/optimizer.hpp>

#include <hpx/runtime/serialization/vector.hpp>

#include <iosfwd>
#include <vector>

namespace allscale {
    enum class schedule_decision {
        done =  0,        // < this task has reached its destination
        stay =  1,        // < stay on current virtual node
        left =  2,         // < send to left child
        right = 3        // < send to right child
    };

    HPX_EXPORT std::ostream& operator<<(std::ostream&, schedule_decision);

    // An abstract base class for scheduling policies...
    struct scheduling_policy
    {
        scheduling_policy()
        {}

        virtual ~scheduling_policy()
        {}

        /**
         * Determines whether the node with the given address is part of the dispatching of a task with the given path.
         *
         * @param addr the address in the hierarchy to be tested
         * @param path the path to be tested
         */
        virtual bool is_involved(const allscale::runtime::HierarchyAddress& addr, const task_id::task_path& path) const = 0;

        /**
         * Obtains the scheduling decision at the given node. The given node must be involved in
         * the scheduling of the given path.
         */
        virtual schedule_decision decide(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const = 0;

        /**
         * Tests whether the given target address is a valid target for the given path
         */
        virtual bool check_target(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const = 0;

    };

    struct decision_tree {

        decision_tree() {}

        HPX_EXPORT decision_tree(std::uint64_t num_nodes);

        // updates a decision for a given path
        HPX_EXPORT void set(task_id::task_path const& path, schedule_decision decision);

        // retrieves the decision for a given path
        HPX_EXPORT schedule_decision get(task_id::task_path const& path) const;

        // --- serialization support ---
        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & encoded_;
        }

    private:
        decision_tree(std::vector<std::uint8_t>&& data) : encoded_(data) {}

        // provide a printer for debugging
        friend std::ostream& operator<<(std::ostream& out, const decision_tree& tree);

        // the encoded form of the decision tree
        // Internally, it is stored in the form of an embedded tree,
        // each node represented by two bits; the two bits are the encoding
        // of the scheduling decision
        std::vector<std::uint8_t> encoded_;
    };

    struct tree_scheduling_policy : scheduling_policy
    {
        tree_scheduling_policy()
        {}

        /**
         * Creates a scheduling policy distributing work on the given scheduling granularity
         * level evenly (as close as possible) among the N available nodes.
         *
         * @param N the number of nodes to distribute work on
         * @param granularity the negative exponent of the acceptable load imbalance; e.g. 0 => 2^0 = 100%, 5 => 2^-5 = 3.125%
         */
        static HPX_EXPORT std::unique_ptr<scheduling_policy> create_uniform(std::vector<bool> const& mask, int granularity);
        static HPX_EXPORT std::unique_ptr<scheduling_policy> create_uniform(std::vector<bool> const& mask);

        static HPX_EXPORT std::unique_ptr<scheduling_policy> create_uniform(int N, int granularity);

        /**
         * Creates a scheduling policy distributing work uniformly among the given number of nodes. The
         * granulartiy will be adjusted accordingly, such that ~8 tasks per node are created.
         *
         * @param N the number of nodes to distribute work on
         */
        static HPX_EXPORT std::unique_ptr<scheduling_policy> create_uniform(int N);

        /**
         * Creates an updated load balancing policy based on a given policy and a measured load distribution.
         * The resulting policy will distributed load evenly among the available nodes, weighted by the observed load.
         *
         * @param old the old policy, based on which the measurement has been taken
         * @param loadDistribution the load distribution measured, utilized for weighting tasks. Ther must be one entry per node,
         *             no entry must be negative.
         */
        static HPX_EXPORT std::unique_ptr<scheduling_policy> create_rebalanced(const scheduling_policy& old, const std::vector<float>& load);

        static HPX_EXPORT std::unique_ptr<scheduling_policy> create_rebalanced(const scheduling_policy& old, const std::vector<float>& load, std::vector<bool> const& mask);
        static HPX_EXPORT std::unique_ptr<scheduling_policy> create_rebalanced(const scheduling_policy& old, task_times const& times, std::vector<bool> const& mask, bool);

        // --- observer ---

        const runtime::HierarchyAddress& root() const
        {
            return root_;
        }

        const decision_tree& tree() const
        {
            return tree_;
        }

        int get_granularity() const
        {
            return granularity_;
        }

        // retrieves the task distribution pattern this tree is realizing
        HPX_EXPORT std::vector<std::size_t> task_distribution_mapping() const;


        // --- the main interface for the scheduler ---

        /**
         * Determines whether the node with the given address is part of the dispatching of a task with the given path.
         *
         * @param addr the address in the hierarchy to be tested
         * @param path the path to be tested
         */
        bool is_involved(const allscale::runtime::HierarchyAddress& addr, const task_id::task_path& path) const;

        /**
         * Obtains the scheduling decision at the given node. The given node must be involved in
         * the scheduling of the given path.
         */
        schedule_decision decide(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const;

        /**
         * Computes the target address a task with the given path should be forwarded to.
         */
        HPX_EXPORT runtime::HierarchyAddress get_target(const task_id::task_path& path) const;

        bool check_target(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const;

//         // --- serialization support ---
//         template <typename Archive>
//         void serialize(Archive& ar, unsigned)
//         {
//             ar & root_;
//             ar & granularity_;
//             ar & tree_;
//         }
        tree_scheduling_policy(runtime::HierarchyAddress root, int granularity, decision_tree&& tree)
          : root_(root)
          , granularity_(granularity)
          , tree_(std::move(tree))
        {}

    private:
        const runtime::HierarchyAddress& getPresumedRootAddress() const {
			return root_;
		}

        runtime::HierarchyAddress root_;
        int granularity_;
        decision_tree tree_;
    };

    struct random_scheduling_policy : scheduling_policy
    {
        random_scheduling_policy(runtime::HierarchyAddress const& root);

        // --- the main interface for the scheduler ---

        /**
         * Determines whether the node with the given address is part of the dispatching of a task with the given path.
         *
         * @param addr the address in the hierarchy to be tested
         * @param path the path to be tested
         */
        bool is_involved(const allscale::runtime::HierarchyAddress& addr, const task_id::task_path& path) const;

        /**
         * Obtains the scheduling decision at the given node. The given node must be involved in
         * the scheduling of the given path.
         */
        schedule_decision decide(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const;


        bool check_target(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const;

    private:
        std::size_t cutoff_level_;
        runtime::HierarchyAddress root_;

        mutable std::uniform_real_distribution<> policy;
        mutable std::random_device generator;
    };
}

#endif
