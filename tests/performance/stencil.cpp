
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

DTYPE weight[2*RADIUS+1][2*RADIUS+1];

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
        //std::cout<<"begin: " << begin << " end: " << end << std::endl;
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
       
        
        std::cout<< " s_src: " << s_src << " e_src: " << e_src << "loc: " << hpx::get_locality_id() << std::endl;
        for(int o = s_src[0]-2; o < e_src[0] + 2; o++){
            for(int t = s_src[1]-2; t < e_src[1] + 2; t++){

                coordinate_type pos_tmp( allscale::utils::Vector<long,2>( o, t ) );
                //std::cout<< data_a[pos_tmp] << " s_src: " << s_src << " e_src: " << e_src << " | ";
                std::cout<< data_a[pos_tmp] << " ";

            }
           std::cout<<std::endl;
        }
        
           std::cout<<std::endl;
           std::cout<<std::endl;
           std::cout<<std::endl;
           std::cout<<std::endl;
        
        //std::cout<<"process for " << s_src << " to " << e_src << std::endl; 
        int tmp,jt,it;
        int jj,ii;
        int i,j;
        for(j = s_src[0]; j < e_src[0]; j++){                                             
            for(i = s_src[1]; i < e_src[1]; i++){
                        coordinate_type pos_src( allscale::utils::Vector<long,2>( i, j ) );
                        
                        //std::cout<< "" <<  pos_src << std::endl;
                        //data_b[pos_src]  = data_a[pos_src];
                        //data_a[pos_src] += 1.0f;
                        for (jj=-RADIUS; jj<=RADIUS; jj++){  
                            //std::cout<<"A" << WEIGHT(0,jj)*data_a[pos_col_neigb]<<" " ; 
                            //data_b[pos_src] += WEIGHT(0,jj)*data_a[pos_col_neigb];
                            coordinate_type pos_col_neigb( allscale::utils::Vector<long,2>( i, j+jj ) );
                            auto tmp_res = WEIGHT(0,jj)*data_a[pos_col_neigb];
                            //std::cout<< "loc: " << hpx::get_locality_id() << " start: " << s_src << " end: " << e_src << "neigb is  " << data_a[pos_col_neigb] << std::endl;
//                            std::cout<< pos_src << " " << data_b[pos_src] << " " << tmp_res << " " << data_a[pos_col_neigb] << std::endl;
                            data_b[pos_src] += tmp_res;
                         //   std::cout<<"BLA" << data_b[pos_src];
                        }

                        for (ii=-RADIUS; ii<0; ii++){
                           // std::cout<<"B" ; 
                            coordinate_type pos_row_neigb( allscale::utils::Vector<long,2>( i+ii, j ) );
                            data_b[pos_src] += WEIGHT(ii,0)*data_a[pos_row_neigb];
                        }
                        for (ii=1; ii<=RADIUS; ii++){       
                            //std::cout<<"C" ;
                            coordinate_type pos_row_neigb( allscale::utils::Vector<long,2>( i+ii, j ) );
                            data_b[pos_src] += WEIGHT(ii,0)*data_a[pos_row_neigb];
                        }

                        //std::cout<< pos_src << " " << data_b[pos_src] << " " << std::endl;

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

        // READ ACCESS REQUIRED TO SRC MATRIX
        //region_type r_src(begin,end);
        
        region_type r_src({begin[0]-RADIUS,begin[1]-RADIUS},{end[0]-1+RADIUS,end[1]-1+RADIUS});

      //  region_type r_src({0,0}, {0,0});
        //std::cout<< "begin: " << begin << " end: "<<  end <<"getting reqs: " << r_src << std::endl;
        //std::cout<< "with ghost region: " << with_ghost_regions << std::endl;
        // READ/WRITE ACCESS REQUIRED TO DST MATRIX
        coordinate_type s_src(hpx::util::get<2>(c));
        coordinate_type e_src(hpx::util::get<3>(c));
        coordinate_type s_dst(allscale::utils::Vector<long,2>( s_src[0], s_src[1] ) );
        coordinate_type e_dst(allscale::utils::Vector<long,2>( e_src[0], e_src[1] ) );
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



        for (jj=-RADIUS; jj<=RADIUS; jj++) {
            for (ii=-RADIUS; ii<=RADIUS; ii++) {
                //std::cout<< WEIGHT(ii,jj) << " ";
            }
           // std::cout<<std::endl;
        }




        allscale::api::user::data::GridPoint<2> size(N,N);
        data_item_shared_data_type sharedData(size);
        
        allscale::data_item_reference<data_item_type> mat_a
            = allscale::data_item_manager::create<data_item_type>(sharedData);

        allscale::data_item_reference<data_item_type> mat_b
            = allscale::data_item_manager::create<data_item_type>(sharedData);


        coordinate_type begin(RADIUS, RADIUS);
        coordinate_type end(N-RADIUS, N-RADIUS);
        region_type whole_region({0,0}, {N, N});

        allscale::spawn_first<grid_init>(mat_a, begin, end, 0).wait();
        allscale::spawn_first<grid_init>(mat_b, begin, end, 1).wait();

         // ============ INIT SOURCE ARRAY WITH COEFFS ==========================
        auto lease_ref_mat_a = allscale::data_item_manager::acquire(
              allscale::createDataItemRequirement(
                  mat_a,
                  whole_region,
                  allscale::access_mode::ReadWrite
        )).get();
        auto ref_mat_a = allscale::data_item_manager::get(mat_a);
        for(int j = 0; j < N; ++j){    
              for(int i = 0; i < N; ++i){
                  coordinate_type tmp(allscale::utils::Vector<long,2>(i,j));
                   ref_mat_a[tmp] = COEFX*i+COEFY*j;
                    //std::cout<<ref_mat_a[tmp]<<" " ;
              }
                std::cout<<std::endl;
        }

        allscale::data_item_manager::release(lease_ref_mat_a);











         
       // DO ACTUAL WORK: SPAWN FIRST WORK ITEM
       hpx::util::high_resolution_timer timer;
       std::cout<<"iterations is : " << iterations << std::endl;
       for(int i = 0 ; i <= iterations; ++i){
           allscale::spawn_first<stencil>(mat_a, mat_b, begin, end).wait();
       }



       //auto startus = std::chrono::high_resolution_clock::now();
   
       //auto endus = std::chrono::high_resolution_clock::now();
       //std::chrono::duration<double> diff = endus-startus;
       //std::cout << "time: " << diff.count() << " s\n";
       double elapsed = timer.elapsed();

       double mups = (((N*N)/(elapsed/iterations))/1000000);
       double combined_matrix_size = 2.0 * sizeof(double) * N * N;
       double mbytes_per_second = combined_matrix_size/(elapsed/iterations) * 1.0E-06;
       std::cout << "Rate (MBs) | avg time (s) | MUP/S: "<<'\n';
       std::cout << mbytes_per_second << "," << elapsed/iterations << "," << mups << '\n';
  /*
       // ============ LOOK AT RESULT==========================
      auto lease_result_a = allscale::data_item_manager::acquire(
          allscale::createDataItemRequirement(
              mat_a,
              whole_region,
              allscale::access_mode::ReadOnly
          )).get();
      auto ref_result_a = allscale::data_item_manager::get(mat_a);
      for(int j = 0; j < N; ++j){    
          for(int i = 0; i < N; ++i){
              coordinate_type tmp(allscale::utils::Vector<long,2>(i,j));
              std::cout<< ref_result_a[tmp] << " ";                                                 

          }
          std::cout<<std::endl;                                                                       
      }
      std::cout<<std::endl;
  */
       // ============ LOOK AT RESULT==========================
       
        double my_norm = 0.0;
       auto lease_result = allscale::data_item_manager::acquire(
          allscale::createDataItemRequirement(
              mat_b,
              whole_region,
              allscale::access_mode::ReadOnly
          )).get();
      auto ref_result = allscale::data_item_manager::get(mat_b);

      for(int j = 0; j < N; ++j){    
          for(int i = 0; i < N; ++i){
              coordinate_type tmp(allscale::utils::Vector<long,2>(i,j));
              my_norm += std::abs(ref_result[tmp]);
             // std::cout<< ref_result[tmp] << " ";                                                 
          }
      }
    // ===================================================
     
     //allscale::data_item_manager::release(lease_result);


        double reference_norm = 0.0;
        allscale::data_item_manager::destroy(mat_a);
        allscale::data_item_manager::destroy(mat_b);

        my_norm =  my_norm / f_active_points;
        reference_norm = (DTYPE) (iterations+1) * (COEFX + COEFY);
        if (std::abs(my_norm-reference_norm) > EPSILON) {
                std::cout<<"ERROR WRONG SOLUTION:" << my_norm << "should be: " << reference_norm<<std::endl;
        }

        return allscale::make_ready_treeture(0);
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv) {
        return allscale::runtime::main_wrapper<main_work>(argc, argv);

        /*
    po::options_description desc("Allowed options");
    desc.add_options()
        ("N", po::value<uint32_t>(), "set number of elements in grid, default = 10000");
    po::variables_map vm;        
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);    
    if (vm.count("N")) {
        cout << "Number of elements was set to " 
             << vm["N"].as<uint32_t>() << ".\n";
        N = vm["N"].as<uint32_t>(); 
    } else {
        cout << "Number of elements was not set, using default = 10000.\n";
    }*/
}
