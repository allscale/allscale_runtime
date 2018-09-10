
#include <allscale/schedule_policy.hpp>

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/vectors.h"

#include <iostream>

namespace allscale {
    namespace {

        int ceilLog2(std::uint64_t x) {
            int i = 0;
            std::uint64_t c = 1;
            while (c<x) {
                c = c << 1;
                i++;
            }
            return i;
        }

        int get_position(task_id::task_path const& p)
        {
            return (1 << p.getLength()) | p.getPath();
        }
    }

    std::ostream& operator<<(std::ostream& out, schedule_decision d)
    {
        switch(d)
        {
            case schedule_decision::done:  return out << "schedule_decision::done";
            case schedule_decision::stay:  return out << "schedule_decision::stay";
            case schedule_decision::left:  return out << "schedule_decision::left";
            case schedule_decision::right: return out << "schedule_decision::right";
            default:
                                           return out << "?";

        }
    }

    decision_tree::decision_tree(std::uint64_t num_nodes)
      : encoded_((2*2*num_nodes/8)) // 2 bits for 2x the number of nodes
    {
        assert_eq((std::uint64_t(1)<<ceilLog2(num_nodes)),num_nodes) << "Number of nodes needs to be a power of 2!";
    }

    // updates a decision for a given path
    void decision_tree::set(task_id::task_path const& path, schedule_decision decision)
    {
        int pos = get_position(path);
        assert_lt(std::size_t(pos/4),encoded_.size());
        int byte_pos = 2*pos / 8;
        int bit_pos = (2*pos) % 8;
        encoded_[byte_pos] = (encoded_[byte_pos] & ~(0x3 << bit_pos)) | (int(decision) << bit_pos);
    }

    // retrieves the decision for a given path
    schedule_decision decision_tree::get(task_id::task_path const& path) const
    {
        // check path length
        int pos = get_position(path);
        int byte_pos = 2*pos / 8;
        int bit_pos = (2*pos) % 8;

        if (std::size_t(byte_pos) > encoded_.size()) return schedule_decision::done;

        // extract encoded value
        return schedule_decision((encoded_[byte_pos] >> bit_pos) & 0x3);
    }

    namespace {

        void printHelper(std::ostream& out, decision_tree const& tree, task_id::task_path const& path) {
            auto d = tree.get(path);
            switch(d)
            {
                case schedule_decision::done: return;
                default:
                    out << path << " => " << d << "\n";
            }
            printHelper(out,tree,path.getLeftChildPath());
            printHelper(out,tree,path.getRightChildPath());
        }

    }

    std::ostream& operator<<(std::ostream& out, const decision_tree& tree) {
        printHelper(out,tree,task_id::task_path::root());
        return out;
    }

    namespace {
		std::vector<std::size_t> getEqualDistribution(int numNodes, int numTasks) {
			std::vector<std::size_t> res(numTasks);

			// fill it with an even load task distribution
			int share = numTasks / numNodes;
			int remainder = numTasks % numNodes;
			int c = 0;
			for(int i=0; i<numNodes; i++) {
				for(int j=0; j<share; j++) {
					res[c++] = i;
				}
				if (i<remainder) {
					res[c++] = i;
				}
			}
			assert_eq(c,numTasks);

			return res;
		}

        void fillTree(decision_tree& tree, task_id::task_path p, int depth, int max_depth, std::size_t range_min, std::size_t range_max, const std::size_t* min, const std::size_t* max) {

			if (depth >= max_depth) return;

			// the center of the task id range
			std::size_t center = range_min + (range_max - range_min)/2;

			// process current
			auto cur_min = *min;
			auto cur_max = (min == max) ? cur_min : *(max-1);


			// the min/max forwarded to the nested call
			std::size_t nested_range_min = range_min;
			std::size_t nested_range_max = range_max;

			// make decision
			if (cur_max < center) {
				depth++;
				tree.set(p,schedule_decision::left);
				nested_range_max = center;
			} else if (center <= cur_min) {
				depth++;
				tree.set(p,schedule_decision::right);
				nested_range_min = center;
			} else {
				tree.set(p,schedule_decision::stay);
			}

			// check whether we are done
			if (depth >= max_depth && min + 1 >= max) return;

			// fill current node

			// cover rest
			auto mid = min + (max - min)/2;

			// fill left
			fillTree(tree,p.getLeftChildPath(),depth,max_depth,nested_range_min,nested_range_max,min,mid);

			// fill right
			fillTree(tree,p.getRightChildPath(),depth,max_depth,nested_range_min,nested_range_max,mid,max);

		}

        decision_tree toDecisionTree(std::size_t num_nodes, const std::vector<std::size_t>& map) {

			// create the resulting decision tree
			auto log2 = ceilLog2(map.size());
			assert_eq((1u<<log2),map.size()) << "Input map is not of power-of-two size: " << map.size();

			// the resulting (unbalanced) tree has to be at most 2x as deep
			decision_tree res((std::uint64_t(1)<<(2*log2)));

			// fill in decisions recursively
			fillTree(res,task_id::task_path::root(), 0, ceilLog2(num_nodes), 0, num_nodes, &*map.begin(),&*map.end());

			// done
			return std::move(res);
		}
    }

    /**
     * Creates a scheduling policy distributing work on the given scheduling granularity
     * level evenly (as close as possible) among the N available nodes.
     *
     * @param N the number of nodes to distribute work on
     * @param granularity the negative exponent of the acceptable load imbalance; e.g. 0 => 2^0 = 100%, 5 => 2^-5 = 3.125%
     */
    std::unique_ptr<scheduling_policy> tree_scheduling_policy::create_uniform(int N, int M, int granularity)
    {
		// some sanity checks
		assert_lt(0,N * M);

		// compute number of levels to be scheduled
		auto log2 = ceilLog2(N * M);
		auto levels = std::max(log2,granularity);

		// create initial task to node mapping
		int numTasks = (1<<levels);
		std::vector<std::size_t> mapping = getEqualDistribution(N * M,numTasks);

		// convert mapping in decision tree
		return std::unique_ptr<scheduling_policy>(new tree_scheduling_policy(
            runtime::HierarchyAddress::getRootOfNetworkSize(N, M), levels,
            toDecisionTree((1<<log2),mapping)));
    }

    /**
     * Creates a scheduling policy distributing work uniformly among the given number of nodes. The
     * granulartiy will be adjusted accordingly, such that ~8 tasks per node are created.
     *
     * @param N the number of nodes to distribute work on
     */
    std::unique_ptr<scheduling_policy> tree_scheduling_policy::create_uniform(int N, int M)
    {
		return create_uniform(N, M, std::max(ceilLog2(N * M)+3,5));
    }

    /**
     * Creates an updated load balancing policy based on a given policy and a measured load distribution.
     * The resulting policy will distributed load evenly among the available nodes, weighted by the observed load.
     *
     * @param old the old policy, based on which the measurement has been taken
     * @param loadDistribution the load distribution measured, utilized for weighting tasks. Ther must be one entry per node,
     * 			no entry must be negative.
     */
    std::unique_ptr<scheduling_policy> tree_scheduling_policy::create_rebalanced(const scheduling_policy& old_base, const std::vector<float>& load)
    {
        tree_scheduling_policy const& old = static_cast<tree_scheduling_policy const&>(old_base);
		// get given task distribution mapping

		// update mapping
		std::vector<std::size_t> mapping = old.task_distribution_mapping();

		// --- estimate costs per task ---

		// get number of nodes
		std::size_t numNodes = 0;
		for(const auto& cur : mapping) {
			numNodes = std::max<std::size_t>(numNodes,cur+1);
		}

		// test that load vector has correct size
		assert_eq(numNodes,load.size())
			<< "Needs current load of all nodes!\n";

		// test that all load values are positive
		assert_true(std::all_of(load.begin(),load.end(),[](float x) { return x >= 0; }))
			<< "Measures loads must be positive!";

		// count number of tasks per node
		std::vector<int> oldShare(numNodes,0);
		for(const auto& cur : mapping) {
			oldShare[cur]++;
		}

		// compute average costs for tasks (default value)
		float sum = 0;
		for(const auto& cur : load) sum += cur;
		float avg = sum / mapping.size();

		// average costs per task on node
		std::vector<float> costs(numNodes);
		for(std::size_t i=0; i<numNodes; i++) {
			if (oldShare[i] == 0) {
				costs[i] = avg;
			} else {
				costs[i] = load[i]/oldShare[i];
			}
		}

		// create vector of costs per task
		float totalCosts = 0;
		std::vector<float> taskCosts(mapping.size());
		for(std::size_t i=0; i<mapping.size(); i++) {
			taskCosts[i] = costs[mapping[i]];
			totalCosts += taskCosts[i];
		}


		// --- redistributing costs ---

		float share = totalCosts / numNodes;


		float curCosts = 0;
		float nextGoal = share;
		std::size_t curNode = 0;
		std::vector<std::size_t> newMapping(mapping.size());
		for(std::size_t i=0; i<mapping.size(); i++) {

			// compute next costs
			auto nextCosts = curCosts + taskCosts[i];

			// if we cross a boundary
			if (curNode < (numNodes-1)) {

				// decide whether to switch to next node
				if (std::abs(curCosts-nextGoal) < std::abs(nextGoal-nextCosts)) {
					// stopping here is closer to the target
					curNode++;
					nextGoal += share;
				}
			}

			// else, just add current task to current node
			newMapping[i] = curNode;
			curCosts = nextCosts;
		}

		// for development, to estimate quality:

		const bool DEBUG_ = false;
		if (DEBUG_) {
			// --- compute new load distribution ---
			std::vector<float> newEstCosts(numNodes,0);
			for(std::size_t i=0; i<mapping.size(); i++) {
				newEstCosts[newMapping[i]] += costs[mapping[i]];
			}

			// compute initial load imbalance variance
			auto mean = [](const std::vector<float>& v)->float {
				float s = 0;
				for(const auto& cur : v) {
					s += cur;
				}
				return s / v.size();
			};
			auto variance = [&](const std::vector<float>& v)->float {
				float m = mean(v);
				float s = 0;
				for(const auto& cur : v) {
					auto d = cur - m;
					s += d*d;
				}
				return s / (v.size()-1);
			};


			std::cout << "Load vector: " << load << " - " << mean(load) << " / " << variance(load) << "\n";
			std::cout << "Est. vector: " << newEstCosts << " - " << mean(newEstCosts) << " / " << variance(newEstCosts) << "\n";
			std::cout << "Target Load: " << share << "\n";
			std::cout << "Task shared: " << oldShare << "\n";
			std::cout << "Node costs:  " << costs << "\n";
			std::cout << "Task costs:  " << taskCosts << "\n";
			std::cout << "In-distribution:  " << mapping << "\n";
			std::cout << "Out-distribution: " << newMapping << "\n";
			std::cout << "\n";
		}

		// create new scheduling policy
		auto root = old.root();
		auto log2 = root.getLayer();
		return std::unique_ptr<scheduling_policy>(new tree_scheduling_policy(
            root, old.granularity_,
            toDecisionTree((1<<log2),newMapping)));
    }

	namespace {

        std::size_t traceTarget(const tree_scheduling_policy& policy, task_id::task_path p) {
			auto res = policy.get_target(p);
            while(!res.isLeaf()) {
//                 assert_lt(p.getLength(),2*policy.getGranularity());
                p = p.getLeftChildPath();
                res = policy.get_target(p);
            }
            return res.getRank();
        }

		void collectPaths(const task_id::task_path& cur, std::vector<task_id::task_path>& res, int depth) {
			if (depth < 0) return;
			res.push_back(cur);
			collectPaths(cur.getLeftChildPath(),res,depth-1);
			collectPaths(cur.getRightChildPath(),res,depth-1);
		}

		std::vector<task_id::task_path> getAllPaths(int depth) {
			std::vector<task_id::task_path> res;
			collectPaths(task_id::task_path::root(),res,depth);
			return res;
		}

	}

    std::vector<std::size_t> tree_scheduling_policy::task_distribution_mapping() const
    {
		std::vector<std::size_t> res;
		res.reserve((1<<granularity_));
		for(const auto& cur : getAllPaths(granularity_)) {
			if (cur.getLength() != granularity_) continue;
			res.push_back(traceTarget(*this,cur));
		}
		return res;
    }

    bool tree_scheduling_policy::is_involved(const allscale::runtime::HierarchyAddress& addr, const task_id::task_path& path) const
    {
		// only the root node is involved in scheduling the root path
		if (path.isRoot()) return addr == root_;

		// TODO: implement this in O(logN) instead of O(logN^2)

		// either the given address is involved in the parent or this node
		return is_involved(addr,path.getParentPath()) || get_target(path) == addr;
    }

    schedule_decision tree_scheduling_policy::decide(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const
    {
		// make sure this address is involved in the scheduling of this task
//		assert_pred2(isInvolved,addr,path);

		// if this is a leaf, there are not that many options :)
		if (addr.isLeaf()) return schedule_decision::stay;

		auto cur = path;
		while(true) {
			// for the root path, the decision is clear
			if (cur.isRoot()) return tree_.get(cur);

			// see whether the addressed node is the node targeted by the parent path
			auto parent = cur.getParentPath();
			if (get_target(parent) == addr) {
				return tree_.get(cur);
			}

			// walk one step further up
			cur = parent;
		}

        HPX_ASSERT(false);
    }

    runtime::HierarchyAddress tree_scheduling_policy::get_target(const task_id::task_path& path) const
    {
        // get location of parent task
        auto res = (path.isRoot()) ? root_ : get_target(path.getParentPath());

        // simulate scheduling
        switch(decide(res,path)) {
            case schedule_decision::done  : return res;
            case schedule_decision::stay  : return res;
            case schedule_decision::left  : return res.getLeftChild();
            case schedule_decision::right : return res.getRightChild();
        }
        assert_fail();
        return res;
    }

    bool tree_scheduling_policy::check_target(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const
    {
        return addr == get_target(path);
    }

    random_scheduling_policy::random_scheduling_policy()
      : cutoff_level_(allscale::get_num_numa_nodes() * allscale::get_num_localities())
    {}


    bool random_scheduling_policy::is_involved(const allscale::runtime::HierarchyAddress& addr, const task_id::task_path& path) const
    {
        std::lock_guard<mutex_type> l(mtx);
        return policy(generator) < 0.2;
    }

    schedule_decision random_scheduling_policy::decide(runtime::HierarchyAddress const& addr, const task_id::task_path& path) const
    {
        std::lock_guard<mutex_type> l(mtx);
        auto r = policy(generator);

        if (r < 0.33)
            return schedule_decision::left;

        if (r < 0.66)
            return schedule_decision::right;

        if (path.getLength() < cutoff_level_)
            return schedule_decision::stay;

        if (r < 0.33)
            return schedule_decision::left;

        return schedule_decision::right;
    }

    bool random_scheduling_policy::check_target(runtime::HierarchyAddress const&, const task_id::task_path&) const
    {
        return true;
    }

}
