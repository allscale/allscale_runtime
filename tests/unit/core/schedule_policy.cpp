#include <hpx/util/lightweight_test.hpp>

#include <type_traits>

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include "allscale/utils/string_utils.h"

#include "allscale/utils/printer/vectors.h"

// #include "allscale/runtime/com/node.h"
// #include "allscale/runtime/com/hierarchy.h"
// #include "allscale/runtime/work/schedule_policy.h"
#include <allscale/schedule_policy.hpp>

void decision_tree_basic()
{
        allscale::decision_tree tree(8);

        allscale::task_id::task_path r = allscale::task_id::task_path::root();
        HPX_TEST(r.isRoot());

        // the default should be done
        HPX_TEST_EQ(allscale::schedule_decision::done,tree.get(r));

        tree.set(r,allscale::schedule_decision::stay);
        HPX_TEST_EQ(allscale::schedule_decision::stay,tree.get(r));

        tree.set(r,allscale::schedule_decision::left);
        HPX_TEST_EQ(allscale::schedule_decision::left,tree.get(r));

        tree.set(r,allscale::schedule_decision::right);
        HPX_TEST_EQ(allscale::schedule_decision::right,tree.get(r));

        tree.set(r,allscale::schedule_decision::done);
        HPX_TEST_EQ(allscale::schedule_decision::done,tree.get(r));

        // try also another position
        auto p = r.getLeftChildPath().getRightChildPath();

        // the default should be done
        HPX_TEST_EQ(allscale::schedule_decision::done,tree.get(p));

        tree.set(p,allscale::schedule_decision::stay);
        HPX_TEST_EQ(allscale::schedule_decision::stay,tree.get(p));

        tree.set(p,allscale::schedule_decision::left);
        HPX_TEST_EQ(allscale::schedule_decision::left,tree.get(p));

        tree.set(p,allscale::schedule_decision::right);
        HPX_TEST_EQ(allscale::schedule_decision::right,tree.get(p));

        tree.set(p,allscale::schedule_decision::done);
        HPX_TEST_EQ(allscale::schedule_decision::done,tree.get(p));

        // something deeper
        p = p.getLeftChildPath().getRightChildPath();
        HPX_TEST_EQ(allscale::schedule_decision::done,tree.get(p));
}

void scheduling_policy_basic()
{
    // type properties
    HPX_TEST(std::is_default_constructible<allscale::scheduling_policy>::value);
    HPX_TEST(std::is_destructible<allscale::scheduling_policy>::value);

    HPX_TEST(std::is_copy_constructible<allscale::scheduling_policy>::value);
    HPX_TEST(std::is_move_constructible<allscale::scheduling_policy>::value);
    HPX_TEST(std::is_copy_assignable<allscale::scheduling_policy>::value);
    HPX_TEST(std::is_move_assignable<allscale::scheduling_policy>::value);
}

namespace {
    void collectPaths(const allscale::task_id::task_path& cur, std::vector<allscale::task_id::task_path>& res, int depth) {
        if (depth < 0) return;
        res.push_back(cur);
        collectPaths(cur.getLeftChildPath(),res,depth-1);
        collectPaths(cur.getRightChildPath(),res,depth-1);
    }

    std::vector<allscale::task_id::task_path> getAll(int depth) {
        std::vector<allscale::task_id::task_path> res;
        collectPaths(allscale::task_id::task_path::root(),res,depth);
        return res;
    }


    allscale::runtime::HierarchyAddress traceTarget(int netSize, int numa, const allscale::scheduling_policy& policy, const allscale::task_id::task_path& path) {
        // for roots it is easy
        if (path.isRoot()) return allscale::runtime::HierarchyAddress::getRootOfNetworkSize(netSize, numa);

        // for everything else, we walk recursive
        auto res = traceTarget(netSize, numa, policy,path.getParentPath());

        // simulate scheduling
        switch(policy.decide(res,path)) {
        case allscale::schedule_decision::done  : return res;
        case allscale::schedule_decision::stay  : return res;
        case allscale::schedule_decision::left  : return res.getLeftChild();
        case allscale::schedule_decision::right : return res.getRightChild();
        }
        assert_fail();
        return res;
    }

    allscale::runtime::HierarchyAddress getTarget(int netSize, int numa, const allscale::scheduling_policy& policy, const allscale::task_id::task_path& path) {

        // trace current path
        auto res = traceTarget(netSize,numa,policy,path);

        // check if this is a leaf-level node
        if (res.isLeaf()) return res.getRank();

        // otherwise, this task is not fully scheduled yet, but its child-tasks will be, and they should all end up at the same point
        std::vector<allscale::task_id::task_path> children;
        collectPaths(path,children,path.getLength());

        // retrieve position of all children, make sure they all reach the same rank
        std::size_t initial = -1;
        std::size_t pos = initial;
        for(const auto& cur : children) {
            if (cur.getLength() != path.getLength()*2) continue;
            auto childTarget = traceTarget(netSize,numa, policy,cur);
            HPX_TEST(childTarget.isLeaf());
            if (pos == initial) pos = childTarget.getRank();
            else
            {
                HPX_TEST_EQ(pos,childTarget.getRank());
            }
        }
        return pos;
    }
}

namespace {

    constexpr int ceilLog2(int x) {
        int i = 0;
        int c = 1;
        while (c<x) {
            c = c << 1;
            i++;
        }
        return i;
    }

}

void scheduling_policy_uniform_fixed()
{
        constexpr int NUM_NODES = 3;
        constexpr int NUM_NUMA = 1;
        constexpr int CEIL_LOG_2_NUM_NODES = ceilLog2(NUM_NODES);
        constexpr int GRANULARITY = 3;

        // get uniform distributed policy
        auto u = allscale::scheduling_policy::create_uniform(NUM_NODES, NUM_NUMA, GRANULARITY);

//        std::cout << u << "\n";

        // get the list of all paths down to the given level
        auto max_length = std::max(CEIL_LOG_2_NUM_NODES,GRANULARITY);
        auto paths = getAll(max_length);

        // collect scheduling target on lowest level
        std::vector<std::size_t> targets;
        for(const auto& cur : paths) {
            HPX_TEST_EQ(traceTarget(NUM_NODES, NUM_NUMA,u,cur),u.get_target(cur));
            if (cur.getLength() != max_length) continue;
            auto target = getTarget(NUM_NODES, NUM_NUMA,u,cur);
            HPX_TEST_EQ(0,target.getLayer());
            targets.push_back(target.getRank());
        }

        HPX_TEST_EQ("[0,0,0,1,1,1,2,2]",toString(targets));
}

void scheduling_policy_uniform_fixed_coarse()
{
        constexpr int NUM_NODES = 3;
        constexpr int NUM_NUMA = 1;
        constexpr int CEIL_LOG_2_NUM_NODES = ceilLog2(NUM_NODES);
        constexpr int GRANULARITY = 2;

        // get uniform distributed policy
        auto u = allscale::scheduling_policy::create_uniform(NUM_NODES,NUM_NUMA, GRANULARITY);

//        std::cout << u << "\n";

        // get the list of all paths down to the given level
        auto max_length = std::max(CEIL_LOG_2_NUM_NODES, GRANULARITY);
        auto paths = getAll(max_length);

        // collect scheduling target on lowest level
        std::vector<std::size_t> targets;
        for(const auto& cur : paths) {
            HPX_TEST_EQ(traceTarget(NUM_NODES,NUM_NUMA, u,cur),u.get_target(cur));
            if (cur.getLength() != max_length) continue;
            auto target = getTarget(NUM_NODES,NUM_NUMA, u,cur);
            HPX_TEST_EQ(0,target.getLayer());
            targets.push_back(target.getRank());
        }

        HPX_TEST_EQ("[0,0,1,2]",toString(targets));

}

void scheduling_policy_uniform_fixed_fine()
{
    constexpr int NUM_NODES = 3;
    constexpr int NUM_NUMA = 1;
    constexpr int CEIL_LOG_2_NUM_NODES = ceilLog2(NUM_NODES);
    constexpr int GRANULARITY = 5;

    // get uniform distributed policy
    auto u = allscale::scheduling_policy::create_uniform(NUM_NODES,NUM_NUMA, GRANULARITY);

//        std::cout << u << "\n";

    // get the list of all paths down to the given level
    auto max_length = std::max(CEIL_LOG_2_NUM_NODES,GRANULARITY);
    auto paths = getAll(max_length);

    // collect scheduling target on lowest level
    std::vector<std::size_t> targets;
    for(const auto& cur : paths) {
        HPX_TEST_EQ(traceTarget(NUM_NODES, NUM_NUMA,u,cur),u.get_target(cur));
        if (cur.getLength() != max_length) continue;
        auto target = getTarget(NUM_NODES, NUM_NUMA, u,cur);
        HPX_TEST_EQ(0,target.getLayer());
        targets.push_back(target.getRank());
    }

    HPX_TEST_EQ("[0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2]",toString(targets));

}

void scheduling_policy_uniform_n3_deeper()
{
    // check larger combination of nodes and extra levels
    for(int n=1; n<16; n++) {
        for(int e = 1; e<=3; e++) {

//             SCOPED_TRACE("n=" + toString(n) + ",e=" + toString(e));

            int NUM_NODES = n;
            constexpr int NUM_NUMA = 1;
            int CEIL_LOG_2_NUM_NODES = ceilLog2(n);
            int GRANULARITY = CEIL_LOG_2_NUM_NODES + e;

            // get uniform distributed policy
            auto u = allscale::scheduling_policy::create_uniform(NUM_NODES,NUM_NUMA, GRANULARITY);

            // get the list of all paths down to the given level
            auto max_length = std::max(CEIL_LOG_2_NUM_NODES,GRANULARITY);
            auto paths = getAll(max_length);

            // collect scheduling target on lowest level
            std::vector<std::size_t> targets;
            for(const auto& cur : paths) {
                HPX_TEST_EQ(traceTarget(NUM_NODES, NUM_NUMA, u,cur),u.get_target(cur));
                if (cur.getLength() != max_length) continue;
                auto target = getTarget(NUM_NODES, NUM_NUMA, u,cur);
                HPX_TEST_EQ(0,target.getLayer());
                targets.push_back(target.getRank());
            }

            // check number of entries
            HPX_TEST_EQ((1<<max_length),targets.size());

            // check that ranks are in range
            for(const auto& cur : targets) {
                HPX_TEST_LTE(0,cur);
                HPX_TEST_LT(cur,n);
            }

            // check that ranks are growing monotone
            for(std::size_t i=0; i<targets.size()-1; i++) {
                HPX_TEST_LTE(targets[i],targets[i+1]);
            }

            // compute a histogram
            std::vector<std::size_t> hist(n,0);
            for(const auto& cur : targets) {
                hist[cur]++;
            }

            // expect distribution +/-1
            auto share = targets.size() / n;
            for(int i=0; i<n; i++) {
                HPX_TEST(hist[i]==share || hist[i] == share+1);
            }
        }
    }

}

namespace {

    // simulates the scheduling processes within the actual task scheduler
    allscale::runtime::HierarchyAddress traceIndirectTarget(const allscale::scheduling_policy& policy, const allscale::runtime::HierarchyAddress& cur, const allscale::task_id::task_path& path) {

        // if current node is not involved, forward to parent
        if (!policy.is_involved(cur,path)) return traceIndirectTarget(policy,cur.getParent(),path);

        // the root node should be at the root location
        if (path.isRoot()) return cur;

        // get location of parent task
        auto parentLoc = traceIndirectTarget(policy,cur,path.getParentPath());

        // check the correctness of this tracer code
        HPX_TEST_EQ(parentLoc,policy.get_target(path.getParentPath()));
        HPX_TEST(policy.is_involved(parentLoc,path));

        // compute where the parent has send this task
        switch(policy.decide(parentLoc,path)) {
        case allscale::schedule_decision::stay: {
            return parentLoc;
        }
        case allscale::schedule_decision::left: {
            HPX_TEST(!parentLoc.isLeaf());
            return parentLoc.getLeftChild();
        }
        case allscale::schedule_decision::right: {
            HPX_TEST(!parentLoc.isLeaf());
            return parentLoc.getRightChild();
        }
        case allscale::schedule_decision::done:
            HPX_TEST(parentLoc.isLeaf());
            return cur;
        }

        assert_fail() << "Invalid decision!";
        return {};
    }

    template<typename Op>
    void forAllChildren(const allscale::runtime::HierarchyAddress& addr, const Op& op) {
        op(addr);
        if (addr.isLeaf()) return;
        forAllChildren(addr.getLeftChild(),op);
        forAllChildren(addr.getRightChild(),op);
    }

    void testAllSources(const allscale::scheduling_policy& policy, const std::string& trg, const allscale::task_id::task_path& path) {
        forAllChildren(policy.root(),[&](const allscale::runtime::HierarchyAddress& cur){
            HPX_TEST_EQ(trg,toString(traceIndirectTarget(policy,cur,path)));
        });
    }

    void testAllSources(const allscale::scheduling_policy& policy, const allscale::runtime::HierarchyAddress& trg, const allscale::task_id::task_path& path) {
        testAllSources(policy,toString(trg),path);
    }
}

void scheduling_policy_redirect()
{
    // start from wrong positions and see whether target can be located successfully

    for(int num_nodes=1; num_nodes<=10; num_nodes++) {

        // get a uniform distribution
        auto policy = allscale::scheduling_policy::create_uniform(num_nodes, 1);

//            std::cout << "N=" << num_nodes << "\n" << policy << "\n";

        // test that all paths can reach their destination starting from any location
        for(const auto& path : getAll(8)) {
            testAllSources(policy,policy.get_target(path),path);
        }

    }
}

void scheduling_policy_balancing()
{
    auto u = allscale::scheduling_policy::create_uniform(4,1,5);

    // providing a nicely balanced load should not cause any changes
    auto loadDist = std::vector<float>(4,1.0);
    auto b1 = allscale::scheduling_policy::create_rebalanced(u,loadDist);
    HPX_TEST_EQ(u.task_distribution_mapping(),b1.task_distribution_mapping());

    // alter the distribution
    loadDist[1] = 3;        // node 1 has 3x more load
    loadDist[3] = 2;        // node 3 has 2x more load
    auto b2 = allscale::scheduling_policy::create_rebalanced(u,loadDist);
    HPX_TEST_NEQ(u.task_distribution_mapping(),b2.task_distribution_mapping());


    // something more homogeneous
    loadDist[0] = 1.25;
    loadDist[1] = 1.5;
    loadDist[2] = 1.25;
    loadDist[3] = 2;
    auto b3 = allscale::scheduling_policy::create_rebalanced(u,loadDist);
    HPX_TEST_NEQ(u.task_distribution_mapping(),b3.task_distribution_mapping());


    // something pretty even
    loadDist[0] = 1.05;
    loadDist[1] = 0.98;
    loadDist[2] = 0.99;
    loadDist[3] = 1.04;
    auto b4 = allscale::scheduling_policy::create_rebalanced(u,loadDist);
    HPX_TEST_EQ(u.task_distribution_mapping(),b4.task_distribution_mapping());



    // test zero-load value
    loadDist[0] = 1.05;
    loadDist[1] = 0;
    loadDist[2] = 0.99;
    loadDist[3] = 1.04;
    auto b5 = allscale::scheduling_policy::create_rebalanced(u,loadDist);
    HPX_TEST_NEQ(u.task_distribution_mapping(),b5.task_distribution_mapping());

}

void scheduling_policy_scaling()
{

    int N = 200;

    // create a policy for N nodes
    auto policy = allscale::scheduling_policy::create_uniform(N, 1);

    std::cout << policy.task_distribution_mapping() << "\n";

}

void scheduling_policy_scaling_rebalancing()
{

    int N = 200;

    // create a policy for N nodes
    auto u = allscale::scheduling_policy::create_uniform(N, 1);

    std::vector<float> load(N,1.0);
    auto b = allscale::scheduling_policy::create_rebalanced(u,load);

    HPX_TEST_EQ(u.task_distribution_mapping(),b.task_distribution_mapping());

}

int hpx_main(int argc, char **argv)
{
    decision_tree_basic();
    scheduling_policy_basic();
    scheduling_policy_uniform_fixed();
    scheduling_policy_uniform_fixed_coarse();
    scheduling_policy_uniform_fixed_fine();
    scheduling_policy_uniform_n3_deeper();
    scheduling_policy_redirect();
    scheduling_policy_balancing();
    scheduling_policy_scaling();
    scheduling_policy_scaling_rebalancing();

    return hpx::finalize();
}

int main(int argc, char **argv)
{
    std::vector<std::string> cfg = {
        "hpx.run_hpx_main!=1",
        "hpx.commandline.allow_unknown!=1"
    };
    hpx::init(argc, argv, cfg);
    return hpx::util::report_errors();
}

