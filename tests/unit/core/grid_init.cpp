
#include <allscale/runtime.hpp>
#include <allscale/api/user/data/static_grid.h>
#include <hpx/util/assert.hpp>
#include <hpx/util/lightweight_test.hpp>

#include <iostream>


#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server.hpp>
#include <allscale/data_item_server_network.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_requirement.hpp>
#include <algorithm>

#include <hpx/config.hpp>
#include <hpx/include/components.hpp>


ALLSCALE_REGISTER_TREETURE_TYPE(int)

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y

HPX_REGISTER_COMPONENT_MODULE();

using data_item_type = allscale::api::user::data::StaticGrid<int, 200, 200>;
using region_type = data_item_type::region_type;
using coordinate_type = data_item_type::coordinate_type;

REGISTER_DATAITEMSERVER_DECLARATION(data_item_type);
REGISTER_DATAITEMSERVER(data_item_type);

////////////////////////////////////////////////////////////////////////////////
// grid init

struct grid_init_name {
    static const char* name() { return "grid_init"; }
};

struct grid_init_split;
struct grid_init_process;
struct grid_init_can_split;

using grid_init = allscale::work_item_description<
    void,
    grid_init_name,
    allscale::do_serialization,
    grid_init_split,
//     allscale::no_split<void>,
    grid_init_process,
    grid_init_can_split
>;

struct grid_init_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

//         std::cout << "can split " << begin << ' ' << end << ' ' << sumOfSquares(end - begin) << '\n';

        return sumOfSquares(end - begin) > 1;
    }
};

struct grid_init_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto data = hpx::util::get<0>(c);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        std::size_t depth = allscale::this_work_item::get_id().depth();
        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(depth, range);

        return allscale::runtime::treeture_parallel(
            allscale::spawn<grid_init>(data, fragments.left.begin(), fragments.left.end()),
            allscale::spawn<grid_init>(data, fragments.right.begin(), fragments.right.end())
        );
    }

    static constexpr bool valid = true;
};

struct grid_init_process {
    template <typename Closure>
    static hpx::util::unused_type execute(Closure const& c)
    {
        auto ref = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        region_type region(begin, end);

        region.scan(
            [&](auto const& pos)
            {
                data[pos] = pos.x + pos.y;
            }
        );

        return hpx::util::unused;
    }

    template <typename Closure>
    static hpx::util::tuple<
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        region_type r(hpx::util::get<1>(c), hpx::util::get<2>(c));
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                r,
                allscale::access_mode::ReadWrite
            )
        );
    }
    static constexpr bool valid = true;
};

////////////////////////////////////////////////////////////////////////////////
// main
struct main_process;

struct main_name {
    static const char* name() { return "main"; }
};

using main_work = allscale::work_item_description<
    int,
    main_name,
    allscale::no_serialization,
    allscale::no_split<int>,
    main_process>;

struct main_process
{
    static allscale::treeture<int> execute(hpx::util::tuple<> const& c)
    {
        allscale::data_item_reference<data_item_type> data
            = allscale::data_item_manager::create<data_item_type>();

        region_type whole_region({0,0}, {200, 200});

        coordinate_type begin(0, 0);
        coordinate_type end(200, 200);

        allscale::spawn<grid_init>(data, begin, end).wait();

        {
            auto lease = allscale::data_item_manager::acquire<data_item_type>(
                allscale::createDataItemRequirement(
                    data,
                    whole_region,
                    allscale::access_mode::ReadOnly)
            );
            auto ref = allscale::data_item_manager::get(data);

            whole_region.scan(
                [&](auto const& pos)
                {
                    HPX_TEST_EQ(ref[pos], pos.x + pos.y);
                }
            );

            allscale::data_item_manager::release(lease);
        }

        allscale::data_item_manager::destroy(data);

        return allscale::make_ready_treeture(0);
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv) {
    allscale::runtime::main_wrapper<main_work>(argc, argv);

    return hpx::util::report_errors();
}
