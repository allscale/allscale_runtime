
//#define ALLSCALE_DEBUG_DIM

#include <allscale/runtime.hpp>
#include <allscale/api/user/data/grid.h>
#include <hpx/util/assert.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>        // std::abs


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

using DTYPE = double;
#define WEIGHT(ii,jj) weight[ii+RADIUS][jj]

constexpr double EPSILON = 1.e-8;
constexpr double COEFX = 1.0;
constexpr double COEFY = 1.0;

HPX_REGISTER_COMPONENT_MODULE();

constexpr int RADIUS = 2;
DTYPE weight[2*RADIUS+1][2*RADIUS+1] =
    {
        {-0.125, 0.00, 0.000,  0.000,  0.00},
        {-0.250, 0.00, 0.000, -0.125, -0.25},
        { 0.000, 0.25, 0.125,  0.000,  0.00},
        { 0.250, 0.00, 0.000,  0.000,  0.00},
        { 0.125, 0.00, 0.000,  0.000,  0.00}
    };


using data_item_type = allscale::api::user::data::Grid<double, 2>;
using region_type = data_item_type::region_type;
using coordinate_type = data_item_type::coordinate_type;
using data_item_shared_data_type = typename data_item_type::shared_data_type;

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
        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);

        HPX_ASSERT(end[0] - begin[0] >= 5);

        return end[0] - begin[0] > 10;
    }
};

struct grid_init_split {
    template <typename Closure>
    static hpx::future<void> execute(Closure const& c)
    {
        auto data_a = hpx::util::get<0>(c);
        auto data_b = hpx::util::get<1>(c);
        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);

        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto diff = end - begin;
        int dim = 1;
        if (diff[0] > diff[1]) dim = 0;
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(dim, range);

        return hpx::when_all(
            allscale::spawn<grid_init>(data_a, data_b, fragments.left.begin(), fragments.left.end()).get_future(),
            allscale::spawn<grid_init>(data_a, data_b, fragments.right.begin(), fragments.right.end()).get_future()
        );
    }

    static constexpr bool valid = true;
};

struct grid_init_process {
    template <typename Closure>
    static hpx::util::unused_type execute(Closure const& c)
    {
        auto data_a = allscale::data_item_manager::get(hpx::util::get<0>(c));
        auto data_b = allscale::data_item_manager::get(hpx::util::get<1>(c));
        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);

        region_type region(begin, end);

        region.scan(
            [&data_a, &data_b](auto const& pos)
            {
                data_a[pos] = COEFX*pos[0]+COEFY*pos[1];
                data_b[pos] = 0.0;
            }
        );

        return hpx::util::unused;
    }

    template <typename Closure>
    static hpx::util::tuple<
        allscale::data_item_requirement<data_item_type >,
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                region_type(hpx::util::get<2>(c), hpx::util::get<3>(c)),
                allscale::access_mode::ReadWrite
            ),
            allscale::createDataItemRequirement(
                hpx::util::get<1>(c),
                region_type(hpx::util::get<2>(c), hpx::util::get<3>(c)),
                allscale::access_mode::ReadWrite
            )
        );
    }
    static constexpr bool valid = true;
};


////////////////////////////////////////////////////////////////////////////////
// grid validate

struct grid_validate_name {
    static const char* name() { return "grid_validate"; }
};

struct grid_validate_split;
struct grid_validate_process;
struct grid_validate_can_split;

using grid_validate = allscale::work_item_description<
    double,
    grid_validate_name,
    allscale::do_serialization,
    grid_validate_split,
    grid_validate_process,
    grid_validate_can_split
>;

struct grid_validate_can_split
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

struct grid_validate_split {
    template <typename Closure>
    static allscale::treeture<double> execute(Closure const& c)
    {
        auto data = hpx::util::get<0>(c);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto N = hpx::util::get<3>(c);

        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto diff = end - begin;
        int dim = 1;
        if (diff[0] > diff[1]) dim = 0;
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(dim, range);

        return hpx::dataflow(
            [](hpx::future<double> a, hpx::future<double> b)
            {
                return a.get() + b.get();
            },
            allscale::spawn<grid_validate>(data, fragments.left.begin(), fragments.left.end(), N).get_future(),
            allscale::spawn<grid_validate>(data, fragments.right.begin(), fragments.right.end(), N).get_future()
        );

    }

    static constexpr bool valid = true;
};

struct grid_validate_process {
    template <typename Closure>
    static double execute(Closure const& c)
    {
        auto ref = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto N = hpx::util::get<3>(c);

        // Clip region
        if (begin[0] < RADIUS) begin[0] = RADIUS;
        if (begin[1] < RADIUS) begin[1] = RADIUS;
        if (end[0] > N - RADIUS) end[0] = N - RADIUS;
        if (end[1] > N - RADIUS) end[1] = N - RADIUS;

        region_type region(begin, end);
        double tmp_res = 0;
        region.scan([&data, &tmp_res](coordinate_type const& pos)
        {
            tmp_res += std::abs(data[pos]);
        });
        return tmp_res;
    }

    template <typename Closure>
    static hpx::util::tuple<
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        auto N = hpx::util::get<3>(c);

        // Clip region
        if (begin[0] < RADIUS) begin[0] = RADIUS;
        if (begin[1] < RADIUS) begin[1] = RADIUS;
        if (end[0] > N - RADIUS) end[0] = N - RADIUS;
        if (end[1] > N - RADIUS) end[1] = N - RADIUS;

        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                region_type(begin, end),
                allscale::access_mode::ReadOnly
            )
        );
    }
    static constexpr bool valid = true;
};

////////////////////////////////////////////////////////////////////////////////
// stencil

struct stencil_name {
    static const char* name() { return "stencil"; }
};

struct stencil_split;
struct stencil_process;
struct stencil_can_split;

using stencil = allscale::work_item_description<
    void,
    stencil_name,
    allscale::do_serialization,
    stencil_split,
    stencil_process,
    stencil_can_split
>;

struct stencil_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);

        HPX_ASSERT(end[0] - begin[0] >= 5);

        return end[0] - begin[0] > 10;
    }
};

struct stencil_split {
    template <typename Closure>
    static hpx::future<void> execute(Closure const& c)
    {
        auto mat_a = hpx::util::get<0>(c);
        auto mat_b = hpx::util::get<1>(c);

        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);
        auto N = hpx::util::get<4>(c);

        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto diff = end - begin;
        int dim = 1;
        if (diff[0] > diff[1]) dim = 0;
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(dim, range);

        return hpx::when_all(
            allscale::spawn<stencil>(mat_a, mat_b, fragments.left.begin(), fragments.left.end(), N).get_future(),
            allscale::spawn<stencil>(mat_a, mat_b, fragments.right.begin(), fragments.right.end(), N).get_future()
        );
    }

    static constexpr bool valid = true;
};

struct stencil_process {
    template <typename Closure>
    static hpx::util::unused_type execute(Closure const& c)
    {
        auto ref_mat_a = hpx::util::get<0>(c);
        auto data_a = allscale::data_item_manager::get(ref_mat_a);

        auto ref_mat_b = hpx::util::get<1>(c);
        auto data_b = allscale::data_item_manager::get(ref_mat_b);

        coordinate_type begin(hpx::util::get<2>(c));
        coordinate_type end(hpx::util::get<3>(c));

        auto N = hpx::util::get<4>(c);

        // Clip region
        if (begin[0] < RADIUS) begin[0] = RADIUS;
        if (begin[1] < RADIUS) begin[1] = RADIUS;
        if (end[0] > N - RADIUS) end[0] = N - RADIUS;
        if (end[1] > N - RADIUS) end[1] = N - RADIUS;

        region_type region(begin, end);
        region.scan([&](coordinate_type const& out_pos)
        {
            for (int j = -RADIUS; j <= RADIUS; ++j)
            {
                coordinate_type in_pos(out_pos[0], out_pos[1] + j);
                data_b[out_pos] += WEIGHT(0, j) * data_a[in_pos];
            }
            for (int i = -RADIUS; i <= RADIUS; ++i)
            {
                coordinate_type in_pos(out_pos[0] + i, out_pos[1]);
                data_b[out_pos] += WEIGHT(i, 0) * data_a[in_pos];
            }
        });
        return hpx::util::unused;
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

        auto N = hpx::util::get<4>(c);

        // Clip region
        if (begin[0] < RADIUS) begin[0] = RADIUS;
        if (begin[1] < RADIUS) begin[1] = RADIUS;
        if (end[0] > N - RADIUS) end[0] = N - RADIUS;
        if (end[1] > N - RADIUS) end[1] = N - RADIUS;

        //acquire regions now
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                region_type(
                    {begin[0]-RADIUS,begin[1]-RADIUS},
                    {end[0]+RADIUS,end[1]+RADIUS}
                ),
                allscale::access_mode::ReadOnly
            ),
            allscale::createDataItemRequirement(
                hpx::util::get<1>(c),
                region_type(begin, end),
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
    static int execute(hpx::util::tuple<int,char**> const& c)
    {
        int argc = hpx::util::get<0>(c);
        char** argv = hpx::util::get<1>(c);
        long iterations = 100;
        long N = 10000;

        iterations = std::atoi(argv[1]);
        N = std::atoi(argv[2]);

        DTYPE f_active_points = (DTYPE) (N-2*RADIUS)*(DTYPE) (N-2*RADIUS);

        int ii,jj;

        for (jj=-RADIUS; jj<=RADIUS; jj++) {
            for (ii=-RADIUS; ii<=RADIUS; ii++) {
                WEIGHT(ii,jj) = (DTYPE) 0.0;
            }
        }
        for (ii=1; ii<=RADIUS; ii++) {
            WEIGHT(0, ii) = WEIGHT( ii,0) =  (DTYPE) (1.0/(2.0*ii*RADIUS));
            WEIGHT(0,-ii) = WEIGHT(-ii,0) = -(DTYPE) (1.0/(2.0*ii*RADIUS));
        }

        int stencil_size = 4*RADIUS+1;

        std::cout
            << "Stencil AllScale benchmark:\n"
            << "\t    localities = " << hpx::get_num_localities().get() << "\n"
            << "\tcores/locality = " << hpx::get_os_thread_count() << "\n"
            << "\t    iterations = " << iterations << "\n"
            << "\t             N = " << N << "\n"
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

        allscale::spawn_first<grid_init>(mat_a, mat_b, begin,end).wait();

        std::cout << "Starting iterations\n";

        // DO ACTUAL WORK: SPAWN FIRST WORK ITEM
        hpx::util::high_resolution_timer timer;
        for(int i = 0 ; i <= iterations; ++i){
            allscale::spawn_first<stencil>(mat_a, mat_b, begin, end, N).wait();
            std::cout << "Iteration " << i << " done.\n";
            if (i == 0)
            {
                std::cout << "First iteration time: " << timer.elapsed() << '\n';
                timer.restart();
            }
        }

        double elapsed = timer.elapsed();

        double flops = ( 2 * stencil_size + 1 ) * f_active_points;
        double avgtime = elapsed/iterations;

        std::cout << "Rate (MFlops/s): " << flops/avgtime * 1.0E-06 << " Avg time (s): " << avgtime << '\n';

        //VALIDATE RESULTS
        allscale::treeture<double> vld = allscale::spawn_first<grid_validate>(mat_b, begin, end, N);

        double res = vld.get_result();

        res = res / f_active_points;
        double reference_norm = 0.0;
        allscale::data_item_manager::destroy(mat_a);
        allscale::data_item_manager::destroy(mat_b);

        reference_norm = (DTYPE) (iterations+1) * (COEFX + COEFY);
        if (std::abs(res-reference_norm) > EPSILON) {
            std::cout<<"ERROR WRONG SOLUTION:" << res << "should be: " << reference_norm<<std::endl;
            return 1;
        }

        std::cout << "Solution validates\n";

        return 0;
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv) {
    return allscale::runtime::main_wrapper<main_work>(argc, argv);
}
