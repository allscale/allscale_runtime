/**
 * ------------------------ Auto-generated Code ------------------------
 *           This code was generated by the Insieme Compiler
 * ---------------------------------------------------------------------
 */
#include <alloca.h>
#include <allscale/api/user/data/scalar.h>
#include <allscale/runtime.hpp>
#include <allscale/no_split.hpp>
#include <allscale/utils/assert.h>

#include <hpx/util/assert.hpp>
#include <hpx/util/lightweight_test.hpp>

#include <iostream>


#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_requirement.hpp>
#include <algorithm>

#include <hpx/config.hpp>
#include <hpx/include/components.hpp>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/input_archive.hpp>
#include <hpx/runtime/serialization/output_archive.hpp>
#include <hpx/runtime/serialization/vector.hpp>

ALLSCALE_REGISTER_TREETURE_TYPE(int32_t)

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y

HPX_REGISTER_COMPONENT_MODULE();

using data_item_type = allscale::api::user::data::Scalar<int32_t>;

REGISTER_DATAITEMSERVER_DECLARATION(data_item_type);
REGISTER_DATAITEMSERVER(data_item_type);

////////////////////////////////////////////////////////////////////////////////
// scalar read

struct scalar_read_name {
    static const char* name() { return "scalar_read"; }
};

struct scalar_read_process;

using scalar_read_work = allscale::work_item_description<
    void,
    scalar_read_name,
    allscale::no_serialization,
    allscale::no_split<void>,
    scalar_read_process
>;

struct scalar_read_process {
    template <typename Closure>
    static hpx::util::unused_type execute(Closure const& c)
    {
        auto s = hpx::util::get<0>(c);

        allscale::data_item_manager::get(s).set(12);

        return hpx::util::unused;
    }

    template <typename Closure>
    static hpx::util::tuple<
        //allscale::data_item_requirement<allscale::api::user::data::Scalar<int32_t > >
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                allscale::api::user::data::detail::ScalarRegion(true),
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
        allscale::data_item_reference<data_item_type> s
            = allscale::data_item_manager::create<data_item_type>();

        allscale::spawn<scalar_read_work >(s).wait();

        {
            auto lease = allscale::data_item_manager::acquire<data_item_type>(
                allscale::createDataItemRequirement(
                    s,
                    allscale::api::user::data::detail::ScalarRegion(true),
                    allscale::access_mode::ReadOnly)
            );
            auto data = allscale::data_item_manager::get(s);
            HPX_TEST_EQ(data.get(), 12);
            allscale::data_item_manager::release(lease);
        }

        return allscale::make_ready_treeture(0);
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv) {
    allscale::runtime::main_wrapper<main_work>(argc, argv);

    return hpx::util::report_errors();
}
