
#include <allscale/runtime.hpp>
#include <allscale/api/user/data/static_grid.h>
#include <hpx/util/assert.hpp>
#include <hpx/util/lightweight_test.hpp>

#include <iostream>

#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_requirement.hpp>
#include <algorithm>

#include <hpx/config.hpp>


ALLSCALE_REGISTER_TREETURE_TYPE(int)

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y

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

        HPX_ASSERT(end[0] - begin[0] >= 5);

        return end[0] - begin[0] > 10;
    }
};

struct grid_init_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto data = hpx::util::get<0>(c);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        auto mid = begin[0] + (end[0] - begin[0]) / 2;

        auto left_end = end; left_end[0] = mid;
        auto right_begin = begin; right_begin[0] = mid;

        return allscale::runtime::treeture_parallel(
            allscale::spawn<grid_init>(data, begin, left_end),
            allscale::spawn<grid_init>(data, right_begin, end)
        );
    }

    static constexpr bool valid = true;
};

struct grid_init_process {
    template <typename Closure>
    static hpx::util::unused_type execute(Closure const& c)
    {
        auto const& ref = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        region_type region(begin, end);

        std::cout << hpx::get_locality_id() << " init: running from " << begin << " to " << end << '\n';

        region.scan(
            [&](auto const& pos)
            {
//                 std::cout << "Setting " << pos << ' ' << pos.x + pos.y << '\n';
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
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        region_type r(begin, end);
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


struct grid_init_test_name {
    static const char* name() { return "grid_init_test"; }
};
struct grid_init_test_split;
struct grid_init_test_process;
struct grid_init_test_can_split;

using grid_init_test = allscale::work_item_description<
    void,
    grid_init_test_name,
    allscale::do_serialization,
    grid_init_test_split,
    grid_init_test_process,
    grid_init_test_can_split
>;

struct grid_init_test_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        HPX_ASSERT(end[0] - begin[0] >= 5);

        return end[0] - begin[0] > 10;
    }
};

struct grid_init_test_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto data = hpx::util::get<0>(c);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        auto mid = begin[0] + (end[0] - begin[0]) / 2;

        auto left_end = end; left_end[0] = mid;
        auto right_begin = begin; right_begin[0] = mid;

        return allscale::runtime::treeture_parallel(
            allscale::spawn<grid_init_test>(data, begin, left_end),
            allscale::spawn<grid_init_test>(data, right_begin, end)
        );
    }

    static constexpr bool valid = true;
};

struct grid_init_test_process {
    template <typename Closure>
    static hpx::util::unused_type execute(Closure const& c)
    {
        auto const& ref = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        region_type region(begin, end);

        std::cout << hpx::get_locality_id() << " test: running from " << begin << " to " << end << '\n';

        bool correct = true;
        region.scan(
            [&](auto const& pos)
            {
                HPX_TEST_EQ(data[pos], pos.x + pos.y);
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
                allscale::access_mode::ReadOnly
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
    static allscale::treeture<int> execute(hpx::util::tuple<> const&)
    {
        allscale::data_item_reference<data_item_type> data
            = allscale::data_item_manager::create<data_item_type>();

        region_type whole_region({0,0}, {200, 200});

        coordinate_type begin(0, 0);
        coordinate_type end(200, 200);

        allscale::spawn_first<grid_init>(data, begin, end).wait();

        allscale::spawn_first<grid_init_test>(data, begin, end).wait();

        auto req = hpx::util::make_tuple(allscale::createDataItemRequirement(
                data,
                whole_region,
                allscale::access_mode::ReadOnly
            ));

        auto location_infos = hpx::util::unwrap(allscale::data_item_manager::locate(req));

        auto lease = hpx::util::unwrap(allscale::data_item_manager::acquire(req,
            location_infos));

        auto ref = allscale::data_item_manager::get(data);

        whole_region.scan(
            [&](auto const& pos)
            {
                if (ref[pos] != pos.x + pos.y)
                {
                    std::cout << pos << '\n';
                    std::abort();
                }
                HPX_TEST_EQ(ref[pos], pos.x + pos.y);
            }
        );

        allscale::data_item_manager::release(lease);

        return allscale::make_ready_treeture(0);
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv) {
    allscale::runtime::main_wrapper<main_work>(argc, argv);

    return hpx::util::report_errors();
}
