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
//     HPX_TEST(std::is_default_constructible<allscale::scheduling_policy>::value);
//     HPX_TEST(std::is_destructible<allscale::scheduling_policy>::value);
//
//     HPX_TEST(std::is_copy_constructible<allscale::scheduling_policy>::value);
//     HPX_TEST(std::is_move_constructible<allscale::scheduling_policy>::value);
//     HPX_TEST(std::is_copy_assignable<allscale::scheduling_policy>::value);
//     HPX_TEST(std::is_move_assignable<allscale::scheduling_policy>::value);
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


    allscale::runtime::HierarchyAddress traceTarget(int netSize, const allscale::scheduling_policy& policy, const allscale::task_id::task_path& path) {
        // for roots it is easy
        if (path.isRoot()) return allscale::runtime::HierarchyAddress::getRootOfNetworkSize(netSize);

        // for everything else, we walk recursive
        auto res = traceTarget(netSize, policy,path.getParentPath());

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

    allscale::runtime::HierarchyAddress getTarget(int netSize, const allscale::scheduling_policy& policy, const allscale::task_id::task_path& path) {

        // trace current path
        auto res = traceTarget(netSize,policy,path);

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
            auto childTarget = traceTarget(netSize, policy,cur);
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
        constexpr int CEIL_LOG_2_NUM_NODES = ceilLog2(NUM_NODES);
        constexpr int GRANULARITY = 3;

        // get uniform distributed policy
        auto up = allscale::tree_scheduling_policy::create_uniform(NUM_NODES, GRANULARITY);
        auto& u = static_cast<allscale::tree_scheduling_policy&>(*up);

//        std::cout << u << "\n";

        // get the list of all paths down to the given level
        auto max_length = std::max(CEIL_LOG_2_NUM_NODES,GRANULARITY);
        auto paths = getAll(max_length);

        // collect scheduling target on lowest level
        std::vector<std::size_t> targets;
        for(const auto& cur : paths) {
            HPX_TEST_EQ(traceTarget(NUM_NODES, u,cur),u.get_target(cur));
            if (cur.getLength() != max_length) continue;
            auto target = getTarget(NUM_NODES,u,cur);
            HPX_TEST_EQ(0u,target.getLayer());
            targets.push_back(target.getRank());
        }

        HPX_TEST_EQ("[0,0,0,1,1,1,2,2]",toString(targets));
}

void scheduling_policy_uniform_fixed_coarse()
{
        constexpr int NUM_NODES = 3;
        constexpr int CEIL_LOG_2_NUM_NODES = ceilLog2(NUM_NODES);
        constexpr int GRANULARITY = 2;

        // get uniform distributed policy
        auto up = allscale::tree_scheduling_policy::create_uniform(NUM_NODES, GRANULARITY);
        auto& u = static_cast<allscale::tree_scheduling_policy&>(*up);

//        std::cout << u << "\n";

        // get the list of all paths down to the given level
        auto max_length = std::max(CEIL_LOG_2_NUM_NODES, GRANULARITY);
        auto paths = getAll(max_length);

        // collect scheduling target on lowest level
        std::vector<std::size_t> targets;
        for(const auto& cur : paths) {
            HPX_TEST_EQ(traceTarget(NUM_NODES, u,cur),u.get_target(cur));
            if (cur.getLength() != max_length) continue;
            auto target = getTarget(NUM_NODES,u,cur);
            HPX_TEST_EQ(0u,target.getLayer());
            targets.push_back(target.getRank());
        }

        HPX_TEST_EQ("[0,0,1,2]",toString(targets));

}

void scheduling_policy_uniform_fixed_fine()
{
    constexpr int NUM_NODES = 3;
    constexpr int CEIL_LOG_2_NUM_NODES = ceilLog2(NUM_NODES);
    constexpr int GRANULARITY = 5;

    // get uniform distributed policy
    auto up = allscale::tree_scheduling_policy::create_uniform(NUM_NODES, GRANULARITY);
    auto& u = static_cast<allscale::tree_scheduling_policy&>(*up);

//        std::cout << u << "\n";

    // get the list of all paths down to the given level
    auto max_length = std::max(CEIL_LOG_2_NUM_NODES,GRANULARITY);
    auto paths = getAll(max_length);

    // collect scheduling target on lowest level
    std::vector<std::size_t> targets;
    for(const auto& cur : paths) {
        HPX_TEST_EQ(traceTarget(NUM_NODES,u,cur),u.get_target(cur));
        if (cur.getLength() != max_length) continue;
        auto target = getTarget(NUM_NODES,u,cur);
        HPX_TEST_EQ(0u,target.getLayer());
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
            int CEIL_LOG_2_NUM_NODES = ceilLog2(n);
            int GRANULARITY = CEIL_LOG_2_NUM_NODES + e;

            // get uniform distributed policy
            auto up = allscale::tree_scheduling_policy::create_uniform(NUM_NODES, GRANULARITY);
            auto& u = static_cast<allscale::tree_scheduling_policy&>(*up);

            // get the list of all paths down to the given level
            auto max_length = std::max(CEIL_LOG_2_NUM_NODES,GRANULARITY);
            auto paths = getAll(max_length);

            // collect scheduling target on lowest level
            std::vector<std::size_t> targets;
            for(const auto& cur : paths) {
                HPX_TEST_EQ(traceTarget(NUM_NODES, u,cur),u.get_target(cur));
                if (cur.getLength() != max_length) continue;
                auto target = getTarget(NUM_NODES, u,cur);
                HPX_TEST_EQ(0u,target.getLayer());
                targets.push_back(target.getRank());
            }

            // check number of entries
            HPX_TEST_EQ((1u<<max_length),targets.size());

            // check that ranks are in range
            for(const auto& cur : targets) {
                HPX_TEST_LTE(0u,cur);
                HPX_TEST_LT(int(cur),n);
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
    allscale::runtime::HierarchyAddress traceIndirectTarget(const allscale::tree_scheduling_policy& policy, const allscale::runtime::HierarchyAddress& cur, const allscale::task_id::task_path& path) {

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

    void testAllSources(const allscale::tree_scheduling_policy& policy, const std::string& trg, const allscale::task_id::task_path& path) {
        forAllChildren(policy.root(),[&](const allscale::runtime::HierarchyAddress& cur){
            HPX_TEST_EQ(trg,toString(traceIndirectTarget(policy,cur,path)));
        });
    }

    void testAllSources(const allscale::tree_scheduling_policy& policy, const allscale::runtime::HierarchyAddress& trg, const allscale::task_id::task_path& path) {
        testAllSources(policy,toString(trg),path);
    }
}

void scheduling_policy_redirect()
{
    // start from wrong positions and see whether target can be located successfully

    for(int num_nodes=1; num_nodes<=10; num_nodes++) {

        // get a uniform distribution
        auto up = allscale::tree_scheduling_policy::create_uniform(num_nodes);
        auto& policy = static_cast<allscale::tree_scheduling_policy&>(*up);

//            std::cout << "N=" << num_nodes << "\n" << policy << "\n";

        // test that all paths can reach their destination starting from any location
        for(const auto& path : getAll(8)) {
            testAllSources(policy,policy.get_target(path),path);
        }

    }
}

void scheduling_policy_rebalancing()
{
    auto up = allscale::tree_scheduling_policy::create_uniform(4,5);
    auto& u = static_cast<allscale::tree_scheduling_policy&>(*up);

    // providing a nicely balanced load should not cause any changes
    auto loadDist = std::vector<float>(4,1.0);
    auto bp1 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist);
    auto& b1 = static_cast<allscale::tree_scheduling_policy&>(*bp1);
    HPX_TEST_EQ(u.task_distribution_mapping(),b1.task_distribution_mapping());

    // alter the distribution
    loadDist[1] = 3;        // node 1 has 3x more load
    loadDist[3] = 2;        // node 3 has 2x more load
    auto bp2 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist);
    auto& b2 = static_cast<allscale::tree_scheduling_policy&>(*bp2);
    HPX_TEST_NEQ(u.task_distribution_mapping(),b2.task_distribution_mapping());


    // something more homogeneous
    loadDist[0] = 1.25;
    loadDist[1] = 1.5;
    loadDist[2] = 1.25;
    loadDist[3] = 2;
    auto bp3 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist);
    auto& b3 = static_cast<allscale::tree_scheduling_policy&>(*bp3);
    HPX_TEST_NEQ(u.task_distribution_mapping(),b3.task_distribution_mapping());


    // something pretty even
    loadDist[0] = 1.05;
    loadDist[1] = 0.98;
    loadDist[2] = 0.99;
    loadDist[3] = 1.04;
    auto bp4 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist);
    auto& b4 = static_cast<allscale::tree_scheduling_policy&>(*bp4);
    HPX_TEST_EQ(u.task_distribution_mapping(),b4.task_distribution_mapping());



    // test zero-load value
    loadDist[0] = 1.05;
    loadDist[1] = 0;
    loadDist[2] = 0.99;
    loadDist[3] = 1.04;
    auto bp5 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist);
    auto& b5 = static_cast<allscale::tree_scheduling_policy&>(*bp5);
    HPX_TEST_NEQ(u.task_distribution_mapping(),b5.task_distribution_mapping());

}

void scheduling_policy_resizing()
{
    auto up = allscale::tree_scheduling_policy::create_uniform(4,5);
    auto& u = static_cast<allscale::tree_scheduling_policy&>(*up);

    auto loadDist = std::vector<float>(4, 1.0);
    auto mask = std::vector<bool>(4, true);
    auto bp1 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist, mask);
    auto& b1 = static_cast<allscale::tree_scheduling_policy&>(*bp1);

    HPX_TEST_EQ(u.task_distribution_mapping(), b1.task_distribution_mapping());

    // remove node 2
    mask[2] = false;
    auto bp2 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist, mask);
    auto& b2 = static_cast<allscale::tree_scheduling_policy&>(*bp2);
    HPX_TEST_EQ("[0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,3,3,3]", toString(b2.task_distribution_mapping()));

    // re-enable node 2
    loadDist[2] = 0;
    mask[2] = true;
    auto bp3 = allscale::tree_scheduling_policy::create_rebalanced(b2,loadDist, mask);
    auto& b3 = static_cast<allscale::tree_scheduling_policy&>(*bp3);

    HPX_TEST_EQ(u.task_distribution_mapping(), b3.task_distribution_mapping());
}

void scheduling_policy_resizing_to_zero()
{
    auto up = allscale::tree_scheduling_policy::create_uniform(6,5);
    auto& u = static_cast<allscale::tree_scheduling_policy&>(*up);

    // providing a nicely balanced load should not cause any changes
    auto loadDist = std::vector<float>(6, 1.0);
    auto mask = std::vector<bool>(6, true);
    auto bp1 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist, mask);
    auto& b1 = static_cast<allscale::tree_scheduling_policy&>(*bp1);

    // remove node 2
    mask[2] = false;
    auto bp2 = allscale::tree_scheduling_policy::create_rebalanced(u,loadDist, mask);
    auto& b2 = static_cast<allscale::tree_scheduling_policy&>(*bp2);
    HPX_TEST_EQ("[0,0,0,0,0,0,0,1,1,1,1,1,1,1,3,3,3,3,3,3,4,4,4,4,4,4,5,5,5,5,5,5]",toString(b2.task_distribution_mapping()));

    // remove node 4
    loadDist[2] = 0;
    mask[4] = false;
    auto bp3 = allscale::tree_scheduling_policy::create_rebalanced(b2,loadDist, mask);
    auto& b3 = static_cast<allscale::tree_scheduling_policy&>(*bp3);
    HPX_TEST_EQ("[0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,5,5,5,5,5,5,5,5]",toString(b3.task_distribution_mapping()));

    // remove node 1
    loadDist[4] = 0;
    mask[1] = false;
    auto bp4 = allscale::tree_scheduling_policy::create_rebalanced(b3,loadDist, mask);
    auto& b4 = static_cast<allscale::tree_scheduling_policy&>(*bp4);
    HPX_TEST_EQ("[0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,3,3,5,5,5,5,5,5,5,5,5,5]",toString(b4.task_distribution_mapping()));

    // remove node 5
    loadDist[1] = 0;
    mask[5] = false;
    auto bp5 = allscale::tree_scheduling_policy::create_rebalanced(b4,loadDist, mask);
    auto& b5 = static_cast<allscale::tree_scheduling_policy&>(*bp5);
    HPX_TEST_EQ("[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3]",toString(b5.task_distribution_mapping()));

    // remove node 0
    loadDist[5] = 0;
    mask[0] = false;
    auto bp6 = allscale::tree_scheduling_policy::create_rebalanced(b5,loadDist, mask);
    auto& b6 = static_cast<allscale::tree_scheduling_policy&>(*bp6);
    HPX_TEST_EQ("[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3]",toString(b6.task_distribution_mapping()));
}

void scheduling_policy_scaling()
{
    int N = 200;

    // create a policy for N nodes
    auto policy = allscale::tree_scheduling_policy::create_uniform(N);
    auto& u = static_cast<allscale::tree_scheduling_policy&>(*policy);

    std::cout << u.task_distribution_mapping() << "\n";

}

void scheduling_policy_scaling_rebalancing()
{
    int N = 200;

    // create a policy for N nodes
    auto up = allscale::tree_scheduling_policy::create_uniform(N, 1);
    auto& u = static_cast<allscale::tree_scheduling_policy&>(*up);

    std::vector<float> load(N,1.0);
    auto bp = allscale::tree_scheduling_policy::create_rebalanced(u,load);
    auto& b = static_cast<allscale::tree_scheduling_policy&>(*bp);

    HPX_TEST_EQ(u.task_distribution_mapping(),b.task_distribution_mapping());

}

void root_handling()
{
    // create a setup of 4 nodes, only using first two nodes
    std::vector<bool> mask(4, true);
    mask[3] = false;
    mask[2] = false;

    // create a uniform work distribution among those
    auto policy = allscale::tree_scheduling_policy::create_uniform(mask);
    auto& u = static_cast<allscale::tree_scheduling_policy&>(*policy);

    auto n  = allscale::runtime::HierarchyAddress::getRootOfNetworkSize(4);
    auto nl = n.getLeftChild();
    auto nr = n.getRightChild();
    auto n0 = nl.getLeftChild();
    auto n1 = nl.getRightChild();
    auto n2 = nr.getLeftChild();
    auto n3 = nr.getRightChild();

    // check the root task
    auto p = allscale::task_id::task_path::root();

    // check involved nodes
    HPX_TEST(u.is_involved(n, p));
    HPX_TEST(u.is_involved(nl, p));
    HPX_TEST(!u.is_involved(nr, p));
    HPX_TEST(!u.is_involved(n0, p));
    HPX_TEST(!u.is_involved(n1, p));
    HPX_TEST(!u.is_involved(n2, p));
    HPX_TEST(!u.is_involved(n3, p));

    // check decision on involved nodes
    HPX_TEST_EQ(allscale::schedule_decision::left, u.decide(n, p));
    HPX_TEST_EQ(allscale::schedule_decision::stay, u.decide(nl, p));
    HPX_TEST_EQ(allscale::schedule_decision::stay, u.decide(nr, p));

    // check task .0
    auto pl = p.getLeftChildPath();

    // check involved nodes
    HPX_TEST(u.is_involved(n, pl));
    HPX_TEST(u.is_involved(nl, pl));
    HPX_TEST(!u.is_involved(nr, pl));
    HPX_TEST(u.is_involved(n0, pl));
    HPX_TEST(!u.is_involved(n1, pl));
    HPX_TEST(!u.is_involved(n2, pl));
    HPX_TEST(!u.is_involved(n3, pl));

    // check deciision on involved nodes
    HPX_TEST_EQ(allscale::schedule_decision::left, u.decide(n, pl));
    HPX_TEST_EQ(allscale::schedule_decision::left, u.decide(nl, pl));
    HPX_TEST_EQ(allscale::schedule_decision::stay, u.decide(n0, pl));

    // check task .1
    auto pr = p.getRightChildPath();

    // check involved nodes
    HPX_TEST(u.is_involved(n, pr));
    HPX_TEST(u.is_involved(nl, pr));
    HPX_TEST(!u.is_involved(nr, pr));
    HPX_TEST(!u.is_involved(n0, pr));
    HPX_TEST(u.is_involved(n1, pr));
    HPX_TEST(!u.is_involved(n2, pr));
    HPX_TEST(!u.is_involved(n3, pr));

    // check deciision on involved nodes
    HPX_TEST_EQ(allscale::schedule_decision::left, u.decide(n, pr));
    HPX_TEST_EQ(allscale::schedule_decision::right, u.decide(nl, pr));
    HPX_TEST_EQ(allscale::schedule_decision::stay, u.decide(n1, pr));
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
    scheduling_policy_rebalancing();
    scheduling_policy_resizing();
    scheduling_policy_resizing_to_zero();
    scheduling_policy_scaling();
    scheduling_policy_scaling_rebalancing();
    root_handling();

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

