#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/work_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/work_item_description.hpp>

#include <allscale/data_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>
#include <allscale/fragment.hpp>


#include <unistd.h>
#include <iostream>
//#include <allscale/scheduler.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/include/unordered_map.hpp>


typedef std::int64_t int_type;

typedef allscale::region<int> my_region;
using my_fragment = allscale::fragment<my_region,int>;
typedef allscale::data_item_description<my_region,my_fragment,int> descr;

typedef allscale::data_item<descr> test_data_item;

ALLSCALE_REGISTER_DATA_ITEM_TYPE(descr);
ALLSCALE_REGISTER_TREETURE_TYPE(int_type);

struct simple_variant_simple_class
{
    static constexpr bool valid = true;
    using result_type = std::int64_t;
    template <typename Closure>
    static allscale::treeture<std::int64_t> execute(Closure const& closure)
    {
        test_data_item td_one = hpx::util::get<1>(closure);
        test_data_item td_two = hpx::util::get<2>(closure);
        auto res = (std::int64_t) *(td_one.fragment_.ptr_); 
        auto res2 = (std::int64_t) *(td_two.fragment_.ptr_); 
        return allscale::treeture<std::int64_t>{ res*res2 };
    }
};

struct simple_name
{
    static const char *name()
    {
        return "simple";
    }
};

using ultra_simple_work_item_descr = 
    allscale::work_item_description<
        std::int64_t,
        simple_name,
        allscale::no_serialization,
        allscale::no_split<std::int64_t>,
        simple_variant_simple_class
    >;

bool test_work_item(){
    std::int64_t known_result = 42;
    descr test_descr;
    my_region test_region(2);
    my_fragment frag(test_region,7);
    test_data_item td(hpx::find_here(),test_descr,frag);

    descr test_descr2;
    my_region test_region2(2);
    my_fragment frag2(test_region2,6);
    test_data_item td2(hpx::find_here(),test_descr2,frag2);

    static allscale::this_work_item::id main_id(0);
    allscale::this_work_item::set_id(main_id);

    using result_type = ultra_simple_work_item_descr::result_type;
    allscale::treeture<result_type> trs(hpx::find_here());
    auto test_item = allscale::work_item(true,ultra_simple_work_item_descr(),trs,3,td,td2);
    
    hpx::apply(&allscale::work_item::process, test_item);
    HPX_ASSERT(trs.valid());
    auto res = trs.get_result();
    
    return (known_result==res);
}

bool test_work_items_all_localities(){
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
    
    std::cout<< "starting timer!!" << std::endl;
    hpx::util::high_resolution_timer kernel1time;
    {
        for(int i = 0; i < 10000; ++i){
            
            std::int64_t known_result = i*(i+1);
            descr test_descr;
            my_region test_region(2);
            my_fragment frag(test_region,i);
            test_data_item td(localities[0],test_descr,frag);
            
            descr test_descr2;
            my_region test_region2(2);
            my_fragment frag2(test_region2,i+1);
            test_data_item td2(localities[0],test_descr2,frag2);

            //std::cout<< hpx::naming::get_locality_from_gid((td.get_id()).get_gid()) << std::endl;
             
            static allscale::this_work_item::id main_id(0);
            allscale::this_work_item::set_id(main_id);

            using result_type = ultra_simple_work_item_descr::result_type;
            allscale::treeture<result_type> trs(localities[0]);
            auto test_item = allscale::work_item(true,ultra_simple_work_item_descr(),trs,3,td,td2);
            
            hpx::apply(&allscale::work_item::process, test_item);
            HPX_ASSERT(trs.valid());
            auto res = trs.get_result();
        }
    }    
    double k1time = kernel1time.elapsed();
    std::cout<< "time : " << k1time << std::endl;
    return true;
}



int hpx_main(int argc, char* argv[])
{
    //simple test
    //std::cout << "test_work_item() returnend " << test_work_item() << std::endl;
    //dsitr test
    test_work_items_all_localities();
    std::cout<<"fin"<<std::endl;
    return hpx::finalize();
}


int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}

