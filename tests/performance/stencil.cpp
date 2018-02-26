
#include <allscale/runtime.hpp>
//#include <allscale/api/user/data/static_grid.h>
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

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y
#define DTYPE double
#define WEIGHT(ii,jj) weight[ii+RADIUS][jj]

#define EPSILON 1.e-8
#define COEFX   1.0
#define COEFY   1.0

HPX_REGISTER_COMPONENT_MODULE();

const int RADIUS = 2;

long iterations = 100;
long N = 10000;
long tile_size = 32;

int stencil_size;

DTYPE weight[2*RADIUS+1][2*RADIUS+1] = {   {-0.125,0,0,0,0},
                                            {-0.25,0,0,-0.125,-0.25},
                                            {0,0.25,0.125,0,0},
                                            {0.25,0,0,0,0},
                                            {0.125,0,0,0,0}
                                       };

const double bla [2][2] = {{1,2},{3,4}};
DTYPE  f_active_points; /* interior of grid with respect to stencil            */

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
// grid init source

struct grid_init_src_name {
    static const char* name() { return "grid_init_src"; }
};

struct grid_init_src_split;
struct grid_init_src_process;
struct grid_init_src_can_split;

using grid_init_src = allscale::work_item_description<
    void,
    grid_init_src_name,
    allscale::do_serialization,
    grid_init_src_split,
    grid_init_src_process,
    grid_init_src_can_split
>;

struct grid_init_src_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        return sumOfSquares(end - begin) > 1;
    }
};

struct grid_init_src_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto data = hpx::util::get<0>(c);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto dim = hpx::util::get<3>(c);
        std::size_t depth = allscale::this_work_item::get_id().depth();
        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(dim, range);

        return allscale::runtime::treeture_parallel(
            allscale::spawn<grid_init_src>(data, fragments.left.begin(), fragments.left.end(), dim),
            allscale::spawn<grid_init_src>(data, fragments.right.begin(), fragments.right.end(), dim)
        );
    }

    static constexpr bool valid = true;
};

struct grid_init_src_process {
    template <typename Closure>
    static hpx::util::unused_type execute(Closure const& c)
    {
        auto ref = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        region_type region(begin, end);

        for(int i = begin[0]; i < end[0]; ++i){
            for(int j = begin[1]; j < end[1]; ++j){

                coordinate_type pos( allscale::utils::Vector<long,2>( i, j ) );
                //data[pos] = 1.0;
                data[pos] = COEFX*i+COEFY*j;

            }
        }

        /*
        region.scan(
            [&](auto const& pos)
            {
//                 std::cout << "Setting " << pos << ' ' << pos.x + pos.y << '\n';
                data[pos] =  hpx::get_locality_id();
            }
        );*/

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
        std::size_t depth = allscale::this_work_item::get_id().depth();
        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(dim, range);

        return allscale::runtime::treeture_parallel(
            allscale::spawn<grid_init>(data, fragments.left.begin(), fragments.left.end(), dim),
            allscale::spawn<grid_init>(data, fragments.right.begin(), fragments.right.end(), dim)
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

        for(int i = begin[0]; i < end[0]; ++i){
            for(int j = begin[1]; j < end[1]; ++j){

                coordinate_type pos( allscale::utils::Vector<long,2>( i, j ) );
                data[pos] = 0.0;

            }
        }

        /*
        region.scan(
            [&](auto const& pos)
            {
//                 std::cout << "Setting " << pos << ' ' << pos.x + pos.y << '\n';
                data[pos] =  hpx::get_locality_id();
            }
        );*/

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

        return sumOfSquares(end - begin) > 1;
    }
};

struct grid_validate_split {
    template <typename Closure>
    static allscale::treeture<double> execute(Closure const& c)
    {
        auto data = hpx::util::get<0>(c);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto dim = hpx::util::get<3>(c);
        std::size_t depth = allscale::this_work_item::get_id().depth();
        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(dim, range);
        /*
        std::cout<<"validate split " << std::endl;
        return allscale::runtime::treeture_parallel(
            allscale::spawn<grid_validate>(data, fragments.left.begin(), fragments.left.end(), dim),
            allscale::spawn<grid_validate>(data, fragments.right.begin(), fragments.right.end(), dim)
        );
        */

        return hpx::dataflow(
            [](hpx::future<double> a, hpx::future<double> b)
            {
                return a.get() + b.get();
            },
            allscale::spawn<grid_validate>(data, fragments.left.begin(), fragments.left.end(), dim).get_future(),
            allscale::spawn<grid_validate>(data, fragments.right.begin(), fragments.right.end(), dim).get_future()
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

        region_type region(begin, end);
        double tmp_res = 0;
        for(int j = begin[1]; j < end[1]; ++j){
            for(int i = begin[0]; i < end[0]; ++i){

                coordinate_type pos( allscale::utils::Vector<long,2>( i, j ) );
                tmp_res += std::abs(data[pos]);
                std::cout<< data[pos] << " ";
            }
            std::cout<<std::endl;
        }
        return tmp_res;
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

        return sumOfSquares(end - begin) > 1;
    }
};

struct stencil_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto mat_a = hpx::util::get<0>(c);
        auto mat_b = hpx::util::get<1>(c);


        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);
     //   std::cout<<"SPLIT begin: " << begin << " end: " << end << " " << "loc: " << hpx::get_locality_id() << std::endl;
        std::size_t depth = allscale::this_work_item::get_id().depth();
        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);

        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(0, range);
     //   std::cout<<"spliitting range: " << range <<" fragments in split are " << fragments.left << " and " << fragments.right << std::endl;
        return allscale::runtime::treeture_parallel(
            allscale::spawn<stencil>(mat_a, mat_b, fragments.left.begin(), fragments.left.end()),
            allscale::spawn<stencil>(mat_a, mat_b,  fragments.right.begin(), fragments.right.end())
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

        coordinate_type s_src(hpx::util::get<2>(c));
        coordinate_type e_src(hpx::util::get<3>(c));

        coordinate_type begin(hpx::util::get<2>(c));
        coordinate_type end(hpx::util::get<3>(c));

        int tmp,jt,it;
        int jj,ii;
        int i,j;
        for(j = s_src[1]; j < e_src[1]; j++){
            for(i = s_src[0]; i < e_src[0]; i++){
                        coordinate_type pos_src( allscale::utils::Vector<long,2>( j, i ) );
                        double tmp_res = data_b[pos_src];
                        for (jj=-RADIUS; jj<=RADIUS; jj++){
                            coordinate_type pos_col_neigb( allscale::utils::Vector<long,2>(  j+jj,i ) );
                            data_b[pos_src] += WEIGHT(0,jj)*data_a[pos_col_neigb];
                            tmp_res += WEIGHT(0,jj)*data_a[pos_col_neigb];
                        }

                        for (ii=-RADIUS; ii<0; ii++){
                            coordinate_type pos_row_neigb( allscale::utils::Vector<long,2>( j,i+ii ) );
                            data_b[pos_src] += WEIGHT(ii,0)*data_a[pos_row_neigb];
                            tmp_res += WEIGHT(ii,0)*data_a[pos_row_neigb];
                        }
                        for (ii=1; ii<=RADIUS; ii++){
                            coordinate_type pos_row_neigb( allscale::utils::Vector<long,2>( j,i+ii) );
                            data_b[pos_src] += WEIGHT(ii,0)*data_a[pos_row_neigb];
                            tmp_res += WEIGHT(ii,0)*data_a[pos_row_neigb];
                        }


            }
        }
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

        region_type r_src({begin[1]-RADIUS,begin[0]-RADIUS},{end[1]+RADIUS,end[0]+RADIUS});
        // READ/WRITE ACCESS REQUIRED TO DST MATRIX
        coordinate_type s_src(hpx::util::get<2>(c));
        coordinate_type e_src(hpx::util::get<3>(c));

        coordinate_type s_dst(allscale::utils::Vector<long,2>( s_src[1], s_src[0] ) );
        coordinate_type e_dst(allscale::utils::Vector<long,2>( e_src[1], e_src[0] ) );


        region_type r_dst(s_dst,e_dst);

        //acquire regions now
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                r_src,
                allscale::access_mode::ReadOnly
            ),
            allscale::createDataItemRequirement(
                hpx::util::get<1>(c),
                r_dst,
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
    static allscale::treeture<int> execute(hpx::util::tuple<int,char**> const& c)
    {
        int argc = hpx::util::get<0>(c);
        char** argv = hpx::util::get<1>(c);
        iterations = std::atoi(argv[1]);
        N = std::atoi(argv[2]);
        tile_size = std::atoi(argv[3]);

        f_active_points = (DTYPE) (N-2*RADIUS)*(DTYPE) (N-2*RADIUS);
     //   DTYPE  weight[2*RADIUS+1][2*RADIUS+1];

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

        stencil_size = 4*RADIUS+1;


        allscale::api::user::data::GridPoint<2> size(N,N);
        data_item_shared_data_type sharedData(size);

        allscale::data_item_reference<data_item_type> mat_a
            = allscale::data_item_manager::create<data_item_type>(sharedData);

        allscale::data_item_reference<data_item_type> mat_b
            = allscale::data_item_manager::create<data_item_type>(sharedData);


        coordinate_type begin(RADIUS, RADIUS);
        coordinate_type end(N-RADIUS, N-RADIUS);
        region_type whole_region({0,0}, {N, N});


        coordinate_type begin_init(0, 0);
        coordinate_type end_init(N, N);

        allscale::spawn_first<grid_init_src>(mat_a, begin_init,end_init, 0).wait();
        allscale::spawn_first<grid_init>(mat_b, begin_init, end_init, 1).wait();


       // DO ACTUAL WORK: SPAWN FIRST WORK ITEM
       hpx::util::high_resolution_timer timer;
       for(int i = 0 ; i <= iterations; ++i){
           allscale::spawn_first<stencil>(mat_a, mat_b, begin, end).wait();
       }

       double elapsed = timer.elapsed();

       double flops = ( 2 * stencil_size + 1 ) * f_active_points;
       double avgtime = elapsed/iterations;
       
       std::cout << "Rate (Flops/s) | avg time (s)"<<'\n';
       std::cout << flops/avgtime * 1.0E-06 << "," << avgtime << '\n';

       //VALIDATE RESULTS
      // allscale::treeture<double> vld = allscale::spawn_first<grid_validate>(mat_b, begin, end, 1);

       //double res = vld.get_result();
       double res = 0.0;
       
       auto req = hpx::util::make_tuple(
          allscale::createDataItemRequirement(
              mat_b,
              whole_region,
              allscale::access_mode::ReadOnly));
       auto lease_result = hpx::util::unwrap(allscale::data_item_manager::acquire(req,
            hpx::util::unwrap(allscale::data_item_manager::locate(req))));
        auto ref_result = allscale::data_item_manager::get(mat_b);

        for(int j = 0; j < N; ++j){
            for(int i = 0; i < N; ++i){
                coordinate_type tmp(allscale::utils::Vector<long,2>(i,j));
                res += std::abs(ref_result[tmp]);
            }
        }

        res = res / f_active_points;
        allscale::data_item_manager::release(lease_result);
        double reference_norm = 0.0;
        allscale::data_item_manager::destroy(mat_a);
        allscale::data_item_manager::destroy(mat_b);

        reference_norm = (DTYPE) (iterations+1) * (COEFX + COEFY);
        if (std::abs(res-reference_norm) > EPSILON) {
                std::cout<<"ERROR WRONG SOLUTION:" << res << "should be: " << reference_norm<<std::endl;
        }
        return allscale::make_ready_treeture(0);
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv) {
        return allscale::runtime::main_wrapper<main_work>(argc, argv);
}
