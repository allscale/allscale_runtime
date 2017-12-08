#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/work_item.hpp>
#include <allscale/treeture.hpp>
#include <allscale/work_item_description.hpp>

#include <allscale/treeture.hpp>


#include <unistd.h>
#include <iostream>
//#include <allscale/scheduler.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/include/unordered_map.hpp>

ALLSCALE_REGISTER_TREETURE_TYPE(long)

struct simple_variant_simple_class
{
    static constexpr bool valid = true;
    using result_type = std::int64_t;
    template <typename Closure>
    static allscale::treeture<std::int64_t> execute(Closure const& closure)
    {
        return allscale::treeture<std::int64_t>{ 2 };
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

    static allscale::this_work_item::id main_id(0);
    allscale::this_work_item::set_id(main_id);

    using result_type = ultra_simple_work_item_descr::result_type;
    allscale::treeture<result_type> trs(allscale::parent_arg{});
    auto test_item = allscale::work_item(true,ultra_simple_work_item_descr(),trs);

    allscale::executor_type exec("default");
    hpx::apply([test_item, &exec]() mutable {allscale::this_work_item::set_id(main_id); test_item.process(exec, false);});
    HPX_ASSERT(trs.valid());
    auto res = trs.get_future().get();

    return (known_result==res);
}

bool test_work_items_all_localities(){
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

    std::cout<< "starting timer!!" << std::endl;
    hpx::util::high_resolution_timer kernel1time;
    {
        allscale::executor_type exec("default");
        for(int i = 0; i < 10000; ++i){

            std::int64_t known_result = i*(i+1);

            //std::cout<< hpx::naming::get_locality_from_gid((td.get_id()).get_gid()) << std::endl;

            static allscale::this_work_item::id main_id(0);
            allscale::this_work_item::set_id(main_id);

            using result_type = ultra_simple_work_item_descr::result_type;
            allscale::treeture<result_type> trs(allscale::parent_arg{});
            auto test_item = allscale::work_item(true,ultra_simple_work_item_descr(),trs);

            hpx::apply([test_item, &exec]()mutable{allscale::this_work_item::set_id(main_id); test_item.process(exec, false);});
            HPX_ASSERT(trs.valid());
            auto res = trs.get_future().get();
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

