
#include <allscale/runtime.hpp>
//#include <allscale/api/user/data/static_grid.h>
#include <allscale/api/user/data/grid.h>
#include <hpx/util/assert.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>


#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_requirement.hpp>
#include <algorithm>

#include <hpx/config.hpp>
#include <hpx/include/components.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>

ALLSCALE_REGISTER_TREETURE_TYPE(int)
ALLSCALE_REGISTER_TREETURE_TYPE(double)

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y

HPX_REGISTER_COMPONENT_MODULE();

//using data_item_type_grid = allscale::api::user::data::Grid<int,1>;

//using data_item_type = allscale::api::user::data::StaticGrid<int, N, N>;
using data_item_type = allscale::api::user::data::Grid<double, 2>;
using region_type = data_item_type::region_type;
using coordinate_type = data_item_type::coordinate_type;
using data_item_shared_data_type = typename data_item_type::shared_data_type;

namespace po = boost::program_options;



REGISTER_DATAITEMSERVER_DECLARATION(data_item_type);
REGISTER_DATAITEMSERVER(data_item_type);

using namespace std;

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
        auto dim = hpx::util::get<3>(c);
        auto N = hpx::util::get<4>(c);

        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(0, range);

        return allscale::runtime::treeture_parallel(
            allscale::spawn<grid_init>(data, fragments.left.begin(), fragments.left.end(), dim, N),
            allscale::spawn<grid_init>(data, fragments.right.begin(), fragments.right.end(), dim, N)
        );
    }

    static constexpr bool valid = true;
};

struct grid_init_process {
    template <typename Closure>
    static void execute(Closure const& c)
    {
        auto ref = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto dim = hpx::util::get<3>(c);
        auto N = hpx::util::get<4>(c);

        region_type region(begin, end);

        region.scan([&](auto p)
        {
            if (dim == 1)
                data[p] = 0.0;
            else
                data[p] = p[1] * N + p[0];
        });
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

struct transpose_incr_name {
    static const char* name() { return "transpose_incr"; }
};

struct transpose_incr_process;
struct transpose_incr_split;
struct transpose_incr_can_split;

using transpose_incr = allscale::work_item_description<
    void,
    transpose_incr_name,
    allscale::do_serialization,
    transpose_incr_split,
    transpose_incr_process,
    transpose_incr_can_split
>;

struct transpose_incr_process {
    template <typename Closure>
    static void execute(Closure const& c)
    {
        auto ref_mat = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref_mat);

        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto tile_size = hpx::util::get<3>(c);

//         region_type region(begin, end);
//
//         region.scan([&data](auto p)
//         {
//             data[p] += 1.0;
//         });
        for(std::int64_t i = begin[0]; i < end[0]; i+=tile_size)
        {
            for(std::int64_t j = begin[1]; j < end[1]; j+=tile_size)
            {
                //iterate thru tiles
                for (std::int64_t it=i; it < std::min(end[0],i+tile_size); it++)
                {
                    for (std::int64_t jt=j; jt < std::min(end[1],j+tile_size);jt++)
                    {
                        data[{it, jt}] += 1.0;
                    }
                }
            }
        }
    }

    template <typename Closure>
    static hpx::util::tuple<
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        // READ ACCESS REQUIRED TO SRC MATRIX
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        region_type region(begin, end);

        //acquire regions now
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                region,
                allscale::access_mode::ReadWrite
            )
        );
    }
    static constexpr bool valid = true;
};

struct transpose_incr_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto data = hpx::util::get<0>(c);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto tile_size = hpx::util::get<3>(c);

        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(0, range);

        return allscale::runtime::treeture_parallel(
            allscale::spawn<transpose_incr>(data, fragments.left.begin(), fragments.left.end(), tile_size),
            allscale::spawn<transpose_incr>(data, fragments.right.begin(), fragments.right.end(), tile_size)
        );
    }

    static constexpr bool valid = true;
};

struct transpose_incr_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        return sumOfSquares(end - begin) > 1;
    }
};

////////////////////////////////////////////////////////////////////////////////
// transpose
struct transpose_name {
    static const char* name() { return "transpose"; }
};

struct transpose_split;
struct transpose_process;
struct transpose_can_split;

using transpose = allscale::work_item_description<
    void,
    transpose_name,
    allscale::do_serialization,
    transpose_split,
    transpose_process,
    transpose_can_split
>;

struct transpose_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);

        return sumOfSquares(end - begin) > 1;
    }
};

struct transpose_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto mat_a = hpx::util::get<0>(c);
        auto mat_b = hpx::util::get<1>(c);


        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);
        std::int64_t tile_size = hpx::util::get<4>(c);

        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        //std::cout<<"about to split range : " << range.begin() << " to " << range.end() << std::endl;

        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(0, range);

//         std::cout<<"transpose splitted, got left: " << fragments.left.begin() << " " << fragments.left.end() << " right: " << fragments.right.begin() << " " <<fragments.right.end() << std::endl;
        return allscale::runtime::treeture_parallel(
            allscale::spawn<transpose>(mat_a, mat_b, fragments.left.begin(), fragments.left.end(), tile_size),
            allscale::spawn<transpose>(mat_a, mat_b,  fragments.right.begin(), fragments.right.end(), tile_size)
        );
    }

    static constexpr bool valid = true;
};

struct transpose_process {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto ref_mat_a = hpx::util::get<0>(c);
        auto data_a = allscale::data_item_manager::get(ref_mat_a);

        auto ref_mat_b = hpx::util::get<1>(c);
        auto data_b = allscale::data_item_manager::get(ref_mat_b);

        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);
        std::int64_t tile_size = hpx::util::get<4>(c);

//         std::cout << "transpose from " << begin << " to " << end << "\n";

//         region_type region(begin, end);
//
//         region.scan([&](auto pos)
//         {
//             data_b[pos] += data_a[{pos.y, pos.x}];
//         });


        for(std::int64_t i = begin[0]; i < end[0]; i+=tile_size)
        {
            for(std::int64_t j = begin[1]; j < end[1]; j+=tile_size)
            {
                //iterate thru tiles
                for (std::int64_t it=i; it < std::min(end[0],i+tile_size); it++)
                {
                    for (std::int64_t jt=j; jt < std::min(end[1],j+tile_size);jt++)
                    {
                        data_b[{it, jt}] += data_a[{jt, it}];
                    }
                }
            }
        }

        return allscale::spawn<transpose_incr>(
            ref_mat_a, coordinate_type({begin[1], begin[0]}),
            coordinate_type({end[1], end[0]}), tile_size);
    }

    template <typename Closure>
    static hpx::util::tuple<
        allscale::data_item_requirement<data_item_type >,
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);
        std::int64_t tile_size = hpx::util::get<4>(c);

//         region_type test_src({begin[1], begin[0]}, {end[1], end[0]});
//         region_type test_dst(begin, end);
//         for(std::int64_t i = begin[0]; i < end[0]; i+=tile_size)
//         {
//             for(std::int64_t j = begin[1]; j < end[1]; j+=tile_size)
//             {
//                 //iterate thru tiles
//                 for (std::int64_t it=i; it < std::min(end[0],i+tile_size); it++)
//                 {
//                     for (std::int64_t jt=j; jt < std::min(end[1],j+tile_size);jt++)
//                     {
//                         if (!test_src.boundingBox().covers(coordinate_type{jt, it}))
//                         {
//                             std::cout << "source: " << jt << "," << it << "\n";
//                         }
//                         if (!test_dst.boundingBox().covers(coordinate_type{it, jt}))
//                         {
//                             std::cout << "dest: " << it << "," << jt << "\n";
//                         }
//                     }
//                 }
//             }
//         }

        //acquire regions now
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                // READ ACCESS REQUIRED TO SRC MATRIX
                region_type({begin[1], begin[0]}, {end[1], end[0]}),
                allscale::access_mode::ReadOnly
            ),
            allscale::createDataItemRequirement(
                hpx::util::get<1>(c),
                // WRITE ACCESS REQUIRED TO SRC MATRIX
                region_type(begin, end),
                allscale::access_mode::ReadWrite
            )
        );
    }
    static constexpr bool valid = true;
};

// transpose result check...
struct transpose_check_name {
    static const char* name() { return "transpose_check"; }
};

struct transpose_check_split;
struct transpose_check_process;
struct transpose_check_can_split;

using transpose_check = allscale::work_item_description<
    double,
    transpose_check_name,
    allscale::do_serialization,
    transpose_check_split,
    transpose_check_process,
    transpose_check_can_split
>;

struct transpose_check_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        return sumOfSquares(end - begin) > 1;
    }
};

struct transpose_check_split {
    template <typename Closure>
    static allscale::treeture<double> execute(Closure const& c)
    {
        auto mat = hpx::util::get<0>(c);

        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto iterations = hpx::util::get<3>(c);
        auto N = hpx::util::get<4>(c);

        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        //std::cout<<"about to split range : " << range.begin() << " to " << range.end() << std::endl;

        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(0, range);

//         std::cout<<"check... splitted, got left: " << fragments.left.begin() << " " << fragments.left.end() << " right: " << fragments.right.begin() << " " <<fragments.right.end() << std::endl;
        return allscale::runtime::treeture_combine(
            allscale::spawn<transpose_check>(mat, fragments.left.begin(), fragments.left.end(), iterations, N),
            allscale::spawn<transpose_check>(mat, fragments.right.begin(), fragments.right.end(), iterations, N),
            [](double l, double r) { return l + r; }
        );
    }

    static constexpr bool valid = true;
};

struct transpose_check_process {
    template <typename Closure>
    static double execute(Closure const& c)
    {
        auto ref_mat = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref_mat);

        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto iterations = hpx::util::get<3>(c);
        auto N = hpx::util::get<4>(c);

        region_type region(begin, end);

//         std::cout << "check from " << begin << " to " << end << "\n";

        double abserr = 0.0;
        double addit = ((double)(iterations+1) * (double) (iterations))/2.0;
        region.scan([&region, &abserr, &data, addit, iterations, N](auto p)
            {
                double err = std::abs(data[p] - (double)(
                    (p[0] * N + p[1])*(iterations+1) + addit));

                abserr += err;
            });

        return abserr;
    }

    template <typename Closure>
    static hpx::util::tuple<
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        // READ ACCESS REQUIRED TO SRC MATRIX
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        region_type region(begin, end);

        //acquire regions now
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                region,
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
    static allscale::treeture<int> execute(hpx::util::tuple<int, char**> const& c)
    {
        int argc = hpx::util::get<0>(c);
        char** argv = hpx::util::get<1>(c);
        std::int64_t iterations = 1;
        if (argc > 1)
            iterations = std::atoi(argv[1]);
        std::int64_t N = 10000;
        if (argc > 2)
            N = std::atoi(argv[2]);
        std::int64_t tile_size = 32;
        if (argc > 3)
            tile_size = std::atoi(argv[3]);

        std::cout
            << "Transpose AllScale benchmark:\n"
            << "\t    localities = " << hpx::get_num_localities().get() << "\n"
            << "\tcores/locality = " << hpx::get_os_thread_count() << "\n"
            << "\t    iterations = " << iterations << "\n"
            << "\t             N = " << N << "\n"
            << "\t     tile_size = " << tile_size << "\n"
            << "\n"
            ;

        allscale::api::user::data::GridPoint<2> size(N,N);
        data_item_shared_data_type sharedData(size);

        allscale::data_item_reference<data_item_type> mat_a
            = allscale::data_item_manager::create<data_item_type>(sharedData);

        allscale::data_item_reference<data_item_type> mat_b
            = allscale::data_item_manager::create<data_item_type>(sharedData);


        coordinate_type begin(0, 0);
        coordinate_type end(N, N);


        allscale::spawn_first<grid_init>(mat_a, begin, end, 0, N).wait();

        allscale::spawn_first<grid_init>(mat_b, begin, end, 1, N).wait();

        region_type whole_region({0,0}, {N, N});

        // DO ACTUAL WORK: SPAWN FIRST WORK ITEM
        hpx::util::high_resolution_timer timer;
        //auto startus = std::chrono::high_resolution_clock::now();

        for (std::int64_t i = 0; i <= iterations; ++i)
        {
            if (i == 1)
                timer.restart();
            allscale::spawn_first<transpose>(mat_a, mat_b, begin, end, tile_size).wait();
            std::cerr << "iteration " << i << " done\n";
        }


        double elapsed = timer.elapsed();

        double mups = (((N*N)/(elapsed/iterations))/1000000);
        double combined_matrix_size = 2.0 * sizeof(double) * N * N;
        double mbytes_per_second = combined_matrix_size/(elapsed/iterations) * 1.0E-06;
        std::cout << "Rate (MBs) | avg time (s) | MUP/S: "<<'\n';
        std::cout << mbytes_per_second << "," << elapsed/iterations << "," << mups << '\n';

        double error = allscale::spawn_first<transpose_check>(mat_b, begin, end, iterations, N).get_result();

        if (error < 1.e-8)
        {
            std::cout << "Solution validates.\n";
        }
        else
        {
            std::cout << "Solution error: " << error << "\n";
        }

        allscale::data_item_manager::destroy(mat_a);
        allscale::data_item_manager::destroy(mat_b);

        return allscale::make_ready_treeture(0);
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv) {
    return allscale::runtime::main_wrapper<main_work>(argc, argv);
}
