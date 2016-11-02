
#include <allscale/this_work_item.hpp>

#include <hpx/util/lightweight_test.hpp>

int main()
{

    HPX_TEST_EQ(allscale::this_work_item::get_id().name(), "0");
    allscale::this_work_item::id& id(allscale::this_work_item::get_id());

    allscale::this_work_item::id id0;
    HPX_TEST_EQ(id0.name(), "0.0");
    allscale::this_work_item::id id1;
    HPX_TEST_EQ(id1.name(), "0.1");
    allscale::this_work_item::id id2;
    HPX_TEST_EQ(id2.name(), "0.2");

    allscale::this_work_item::set_id(id0);
    {
        allscale::this_work_item::id id00;
        HPX_TEST_EQ(id00.name(), "0.0.0");
        allscale::this_work_item::id id01;
        HPX_TEST_EQ(id01.name(), "0.0.1");
        allscale::this_work_item::id id02;
        HPX_TEST_EQ(id02.name(), "0.0.2");
    }

    allscale::this_work_item::set_id(id1);
    {
        allscale::this_work_item::id id00;
        HPX_TEST_EQ(id00.name(), "0.1.0");
        allscale::this_work_item::id id01;
        HPX_TEST_EQ(id01.name(), "0.1.1");
        allscale::this_work_item::id id02;
        HPX_TEST_EQ(id02.name(), "0.1.2");
    }

    allscale::this_work_item::set_id(id2);
    {
        allscale::this_work_item::id id00;
        HPX_TEST_EQ(id00.name(), "0.2.0");
        allscale::this_work_item::id id01;
        HPX_TEST_EQ(id01.name(), "0.2.1");
        allscale::this_work_item::id id02;
        HPX_TEST_EQ(id02.name(), "0.2.2");
    }

    allscale::this_work_item::set_id(id);

    allscale::this_work_item::id id3;
    HPX_TEST_EQ(id3.name(), "0.3");
    allscale::this_work_item::id id4;
    HPX_TEST_EQ(id4.name(), "0.4");
    allscale::this_work_item::id id5;
    HPX_TEST_EQ(id5.name(), "0.5");

    return hpx::util::report_errors();
}
