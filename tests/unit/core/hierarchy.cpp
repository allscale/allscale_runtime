
#include <allscale/hierarchy.hpp>

#include <hpx/util/lightweight_test.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include "allscale/utils/string_utils.h"

void type_traits()
{
    using namespace allscale::runtime;

    // some basic type traits
    HPX_TEST(std::is_default_constructible<HierarchyAddress>::value);
    HPX_TEST(std::is_destructible<HierarchyAddress>::value);

    HPX_TEST(std::is_trivially_copy_constructible<HierarchyAddress>::value);
    HPX_TEST(std::is_trivially_copy_assignable<HierarchyAddress>::value);

    HPX_TEST(std::is_trivially_move_constructible<HierarchyAddress>::value);
    HPX_TEST(std::is_trivially_move_assignable<HierarchyAddress>::value);


    // make sure the hierarchy address is serializable
    HPX_TEST(allscale::utils::is_serializable<HierarchyAddress>::value);
    HPX_TEST(allscale::utils::is_trivially_serializable<HierarchyAddress>::value);
}

void create()
{
    using namespace allscale::runtime;
    // test that the default version is valid
    HierarchyAddress addr;
}

void print()
{
    using namespace allscale::runtime;
    // test that the default version is valid
    HierarchyAddress addr;

    HPX_TEST_EQ("0:0", toString(addr));
}

void root()
{
    using namespace allscale::runtime;
    // test the root of various network sizes
    HPX_TEST_EQ("0:0",toString(HierarchyAddress::getRootOfNetworkSize( 1)));
    HPX_TEST_EQ("0:1",toString(HierarchyAddress::getRootOfNetworkSize( 2)));
    HPX_TEST_EQ("0:2",toString(HierarchyAddress::getRootOfNetworkSize( 3)));
    HPX_TEST_EQ("0:2",toString(HierarchyAddress::getRootOfNetworkSize( 4)));
    HPX_TEST_EQ("0:3",toString(HierarchyAddress::getRootOfNetworkSize( 5)));
    HPX_TEST_EQ("0:3",toString(HierarchyAddress::getRootOfNetworkSize( 6)));
    HPX_TEST_EQ("0:3",toString(HierarchyAddress::getRootOfNetworkSize( 7)));
    HPX_TEST_EQ("0:3",toString(HierarchyAddress::getRootOfNetworkSize( 8)));
    HPX_TEST_EQ("0:4",toString(HierarchyAddress::getRootOfNetworkSize( 9)));
    HPX_TEST_EQ("0:4",toString(HierarchyAddress::getRootOfNetworkSize(10)));
    HPX_TEST_EQ("0:4",toString(HierarchyAddress::getRootOfNetworkSize(11)));
    HPX_TEST_EQ("0:4",toString(HierarchyAddress::getRootOfNetworkSize(12)));
    HPX_TEST_EQ("0:4",toString(HierarchyAddress::getRootOfNetworkSize(13)));
    HPX_TEST_EQ("0:4",toString(HierarchyAddress::getRootOfNetworkSize(14)));
    HPX_TEST_EQ("0:4",toString(HierarchyAddress::getRootOfNetworkSize(15)));
    HPX_TEST_EQ("0:4",toString(HierarchyAddress::getRootOfNetworkSize(16)));
    HPX_TEST_EQ("0:5",toString(HierarchyAddress::getRootOfNetworkSize(17)));
}

void layers_on_node()
{
    using namespace allscale::runtime;

    // test the root of various network sizes
    HPX_TEST_EQ(1,HierarchyAddress::getLayersOn(0,1));
    HPX_TEST_EQ(2,HierarchyAddress::getLayersOn(0,2));
    HPX_TEST_EQ(3,HierarchyAddress::getLayersOn(0,3));
    HPX_TEST_EQ(3,HierarchyAddress::getLayersOn(0,4));
    HPX_TEST_EQ(4,HierarchyAddress::getLayersOn(0,5));
    HPX_TEST_EQ(4,HierarchyAddress::getLayersOn(0,6));
    HPX_TEST_EQ(4,HierarchyAddress::getLayersOn(0,7));
    HPX_TEST_EQ(4,HierarchyAddress::getLayersOn(0,8));
    HPX_TEST_EQ(5,HierarchyAddress::getLayersOn(0,9));

    // test other nodes
    HPX_TEST_EQ(4,HierarchyAddress::getLayersOn(0,8));
    HPX_TEST_EQ(1,HierarchyAddress::getLayersOn(1,8));
    HPX_TEST_EQ(2,HierarchyAddress::getLayersOn(2,8));
    HPX_TEST_EQ(1,HierarchyAddress::getLayersOn(3,8));
    HPX_TEST_EQ(3,HierarchyAddress::getLayersOn(4,8));
    HPX_TEST_EQ(1,HierarchyAddress::getLayersOn(5,8));
    HPX_TEST_EQ(2,HierarchyAddress::getLayersOn(6,8));
    HPX_TEST_EQ(1,HierarchyAddress::getLayersOn(7,8));
}

namespace {
    using namespace allscale::runtime;

    void collectAll(const HierarchyAddress& cur, std::set<HierarchyAddress>& res) {
        HPX_TEST(res.insert(cur).second);
        if (cur.isLeaf()) return;
        collectAll(cur.getLeftChild(),res);
        collectAll(cur.getRightChild(),res);
    }

    std::set<HierarchyAddress> getAllAddresses(std::size_t size) {
        std::set<HierarchyAddress> all;
        collectAll(HierarchyAddress::getRootOfNetworkSize(size),all);

        // filter all beyond size
        std::set<HierarchyAddress> res;
        for(const auto& cur : all) {
            if (cur.getRank() < size) res.insert(cur);
        }

        // done
        return res;
    }

}

void navigation()
{
    using namespace allscale::runtime;
    std::size_t N = 8;

    // test the full navigation in an 8-wide tree
    auto root = HierarchyAddress::getRootOfNetworkSize(N);

    // collect all nodes
    std::set<HierarchyAddress> all;
    collectAll(root,all);

    // check the number of nodes
    HPX_TEST_EQ(15,all.size());

    int numLeafs = 0;
    int numInner = 0;
    int numLeftChildren = 0;
    int numRightChildren = 0;
    for(const auto& cur : all) {

        if (cur.isLeaf()) {
            numLeafs++;
        } else {
            HPX_TEST(cur.isVirtualNode());
            numInner++;
        }

        // check family relations
        if (cur.isLeftChild()) {
            HPX_TEST_EQ(cur,cur.getParent().getLeftChild());
            numLeftChildren++;
        } else {
            HPX_TEST(cur.isRightChild());
            HPX_TEST_EQ(cur,cur.getParent().getRightChild());
            numRightChildren++;
        }

        // check height
        HPX_TEST_LT(cur.getLayer(), HierarchyAddress::getLayersOn(cur.getRank(), N));
    }

    // check the correct number of leafs and inner nodes
    HPX_TEST_EQ(8,numLeafs);
    HPX_TEST_EQ(7,numInner);

    // also: the correct number of left and right children
    HPX_TEST_EQ(8,numLeftChildren); // the root is a left child
    HPX_TEST_EQ(7,numRightChildren);
}

using namespace allscale::runtime;
namespace {
	struct LayerService {

		HierarchyAddress myAddress;

		LayerService(const HierarchyAddress& addr) : myAddress(addr) {}

		HierarchyAddress whereAreYou() { return myAddress; }
	};

}
HierarchyAddress where_are_you(HierarchyAddress const& addr)
{
    return HierarchicalOverlayNetwork::getLocalService<LayerService>(addr.getLayer()).whereAreYou();
}

HPX_PLAIN_ACTION(where_are_you)

void comm_test_impl()
{
	auto all = getAllAddresses(allscale::get_num_localities());

    for(const auto& cur : all) {
        hpx::id_type dest = hpx::naming::get_id_from_locality_id(cur.getRank());

        auto reply = hpx::async<where_are_you_action>(dest, cur).get();

        HPX_TEST_EQ(reply,cur);
    }

}

HPX_PLAIN_ACTION(comm_test_impl)

void comm_test()
{
    using namespace allscale::runtime;

    HierarchicalOverlayNetwork hierarchy;

    hierarchy.installService<LayerService>();

    auto localities = hpx::find_remote_localities();

    auto dest = localities.empty() ? hpx::find_here() : localities[localities.size()/2];

    hpx::async<comm_test_impl_action>(dest).get();
}

int hpx_main(int argc, char **argv)
{
    type_traits();
    create();
    print();
    root();
    layers_on_node();
    navigation();
    comm_test();

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
