
#include <allscale/runtime.hpp>
#include <vector>

#include "allscale/api/user/data/grid.h"
#include "allscale/api/user/data/static_grid.h"

ALLSCALE_REGISTER_TREETURE_TYPE(int32_t);

#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server.hpp>
#include <allscale/data_item_server_network.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_requirement.hpp>
#include <algorithm>

#include <hpx/hpx_main.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/config.hpp>
#include <hpx/include/components.hpp>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/input_archive.hpp>
#include <hpx/runtime/serialization/output_archive.hpp>
#include <hpx/runtime/serialization/vector.hpp>
#include <chrono>

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y

using namespace allscale::api::user::data;
using namespace allscale;


// -- example parameter --

const int N = 16384;	// -- size of vector
const int T = 100;	// -- number of time steps



using data_item_type_grid = StaticGrid<int,N,N>;
REGISTER_DATAITEMSERVER_DECLARATION(data_item_type_grid);
REGISTER_DATAITEMSERVER(data_item_type_grid);




// -- initializer work item --

struct init_closure {
	StaticGridPoint<2> a, b;
	int value;
	data_item_reference<StaticGrid<int,N,N>> data;
};

struct __wi_init_name {
	static const char* name() { return "__wi_init"; }
};

struct __wi_init_process {
    static constexpr bool valid = true;

    static auto get_requirements(hpx::util::tuple<init_closure> const& var_0) {
    	StaticGridPoint<2> a = hpx::util::get<0>(var_0).a;
    	StaticGridPoint<2> b = hpx::util::get<0>(var_0).b;
    	const data_item_reference<StaticGrid<int,N,N>>& data = hpx::util::get<0>(var_0).data;

		return hpx::util::make_tuple( createDataItemRequirement(data,StaticGridRegion<2>(a,b),access_mode::ReadWrite) );
	}

    static allscale::treeture<void> execute(hpx::util::tuple<init_closure> const& var_0) {

    	StaticGridPoint<2> a = hpx::util::get<0>(var_0).a;
    	StaticGridPoint<2> b = hpx::util::get<0>(var_0).b;

    	int value = hpx::util::get<0>(var_0).value;

    	auto data = data_item_manager::get(hpx::util::get<0>(var_0).data);

    	StaticGridRegion<2>(a,b).scan([&](const StaticGridPoint<2>& p){
    		data[p] = value;
    	});

    	return allscale::make_ready_treeture();
    }
};

struct __wi_init_split {
    static constexpr bool valid = true;

    static auto get_requirements(hpx::util::tuple<init_closure> const&) {
    	return hpx::util::make_tuple();
    }

    static allscale::treeture<void> execute(hpx::util::tuple<init_closure> const& var_0);
};

using __wi_init_work = allscale::work_item_description<
		void,
		__wi_init_name,
		allscale::no_serialization,
		__wi_init_split,
		__wi_init_process
	>;



allscale::treeture<void> __wi_init_split::execute(hpx::util::tuple<init_closure> const& var_0) {

	StaticGridPoint<2> a = hpx::util::get<0>(var_0).a;
	StaticGridPoint<2> b = hpx::util::get<0>(var_0).b;

	// select the split dimension
	int i = ((b[1] - a[1]) > (b[0] - a[0])) ? 1 : 0;
	int m = a[i] + (b[i]-a[1])/2;

	// compute the split points
	StaticGridPoint<2> m1 = b;
	StaticGridPoint<2> m2 = a;

	m1[i] = m;
	m2[i] = m;

	// std::cout << "Splitting " << a << " - " << b << " in " << a << " - " << m1 << " and " << m2 << " - " << b << "\n";

	int value = hpx::util::get<0>(var_0).value;

	const data_item_reference<StaticGrid<int,N,N>>& data = hpx::util::get<0>(var_0).data;

	return allscale::runtime::treeture_parallel(
		allscale::spawn<__wi_init_work>(init_closure { a, m1, value, data }),
		allscale::spawn<__wi_init_work>(init_closure { m2, b, value, data })
	);
}



// -- stencil update step --

struct stencil_update_closure {
	StaticGridPoint<2> a, b;
	data_item_reference<StaticGrid<int,N,N>> src;
	data_item_reference<StaticGrid<int,N,N>> dst;
};

struct __wi_stencil_update_name {
	static const char* name() { return "__wi_stencil_update"; }
};

struct __wi_stencil_update_process {
    static constexpr bool valid = true;

    static auto get_requirements(hpx::util::tuple<stencil_update_closure> const& var_0) {
    	StaticGridPoint<2> a = hpx::util::get<0>(var_0).a;
    	StaticGridPoint<2> b = hpx::util::get<0>(var_0).b;

    	auto a2 = a;
    	a2[0] -= 1; a2[1] -= 1;

    	auto b2 = b;
		b2[0] += 1; b2[1] += 1;

    	const data_item_reference<StaticGrid<int,N,N>>& src = hpx::util::get<0>(var_0).src;
    	const data_item_reference<StaticGrid<int,N,N>>& dst = hpx::util::get<0>(var_0).dst;

		return hpx::util::make_tuple(
			createDataItemRequirement(src,StaticGridRegion<2>(a2,b2),access_mode::ReadOnly),
			createDataItemRequirement(dst,StaticGridRegion<2>(a,b),access_mode::ReadWrite)
		);
	}

    static hpx::util::unused_type execute(hpx::util::tuple<stencil_update_closure> const& var_0) {

    	StaticGridPoint<2> a = hpx::util::get<0>(var_0).a;
    	StaticGridPoint<2> b = hpx::util::get<0>(var_0).b;

    	auto src = data_item_manager::get(hpx::util::get<0>(var_0).src);
    	auto dst = data_item_manager::get(hpx::util::get<0>(var_0).dst);

    	StaticGridRegion<2>(a,b).scan([&](const StaticGridPoint<2>& p) {

    		using point = StaticGridPoint<2>;

    		// check that neighborhood is at the same stage (time)
    		if (p.x >   0) assert_eq((src[p + point{-1, 0}]),src[p]);
    		if (p.x < N-1) assert_eq((src[p + point{ 1, 0}]),src[p]);
    		if (p.y >   0) assert_eq((src[p + point{ 0,-1}]),src[p]);
    		if (p.y < N-1) assert_eq((src[p + point{ 0, 1}]),src[p]);

    		// update the target time
    		dst[p] = src[p] + 1;

    	});

    	return hpx::util::unused;
    }
};

struct __wi_stencil_update_split {
    static constexpr bool valid = true;

    static auto get_requirements(hpx::util::tuple<stencil_update_closure> const&) {
    	return hpx::util::make_tuple();
    }

    static allscale::treeture<void> execute(hpx::util::tuple<stencil_update_closure> const& var_0);
};

using __wi_stencil_update_work = allscale::work_item_description<
		void,
		__wi_stencil_update_name,
		allscale::no_serialization,
		__wi_stencil_update_split,
		__wi_stencil_update_process
	>;



allscale::treeture<void> __wi_stencil_update_split::execute(hpx::util::tuple<stencil_update_closure> const& var_0) {

	StaticGridPoint<2> a = hpx::util::get<0>(var_0).a;
	StaticGridPoint<2> b = hpx::util::get<0>(var_0).b;

	// select the split dimension
	int i = ((b[1] - a[1]) > (b[0] - a[0])) ? 1 : 0;
	int m = a[i] + (b[i]-a[1])/2;

	// compute the split points
	StaticGridPoint<2> m1 = b;
	StaticGridPoint<2> m2 = a;

	m1[i] = m;
	m2[i] = m;

	const data_item_reference<StaticGrid<int,N,N>>& src = hpx::util::get<0>(var_0).src;
	const data_item_reference<StaticGrid<int,N,N>>& dst = hpx::util::get<0>(var_0).dst;

	return allscale::runtime::treeture_parallel(
		allscale::spawn<__wi_stencil_update_work>(stencil_update_closure { a, m1, src, dst }),
		allscale::spawn<__wi_stencil_update_work>(stencil_update_closure { m2, b, src, dst })
	);
}






int32_t IMP_main() {

	data_item_reference<StaticGrid<int,N,N>> vecA = data_item_manager::create<StaticGrid<int,N,N>>();
	data_item_reference<StaticGrid<int,N,N>> vecB = data_item_manager::create<StaticGrid<int,N,N>>();

	std::cout << "Initializing vectors ..\n";

	auto i1 = allscale::spawn<__wi_init_work>(init_closure { 0, N,  0, vecA });
	auto i2 = allscale::spawn<__wi_init_work>(init_closure { 0, N, -1, vecB });

	i1.get_future().wait();
	i2.get_future().wait();

	std::cout << "Starting simulation loop ..\n";
    auto start = std::chrono::high_resolution_clock::now();
	for(int t=0; t<T; t+=2) {

		// B := f(A)
		allscale::spawn_first<__wi_stencil_update_work>(stencil_update_closure { 0, N, vecA, vecB }).get_future().wait();

		// A := f(B)
		allscale::spawn_first<__wi_stencil_update_work>(stencil_update_closure { 0, N, vecB, vecA }).get_future().wait();

	}

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end-start;
	std::cout << "Time: " << diff.count() << "\n";



	// -- test the resulting status of the data item --
	auto full = StaticGridRegion<2>(0,N);
	auto leaseA = data_item_manager::acquire(createDataItemRequirement(vecA,full,access_mode::ReadOnly));

	auto vecAData = data_item_manager::get(vecA);

	full.scan([&](const StaticGridPoint<2>& p){
		if (vecAData[p] != T) {
			std::cerr << "Invalid value at position i=" << p << ": " << vecAData[p] << " should be " << T << "\n";
			exit(1);
		}
	});

	data_item_manager::release(leaseA);

	data_item_manager::destroy(vecA);
	data_item_manager::destroy(vecB);

	std::cout << "Success!\n";

    return 0;
}

// -- main work item --

allscale::treeture<int32_t > allscale_fun_1(hpx::util::tuple<int, char**> const&) {
    return allscale::treeture<int32_t >(IMP_main());
}

struct __wi_main_name {
    static const char* name() { return "__wi_main"; }
};

struct __wi_main_process {
    static constexpr bool valid = true;
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int, char**> const& var_0) {
    	return allscale_fun_1(var_0);
    }
};

struct __wi_main_split {
    static constexpr bool valid = true;
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int, char**> const& var_0) {
    	return allscale_fun_1(var_0);
    }
};

using __wi_main_work = allscale::work_item_description<
		int32_t,
		__wi_main_name,
		allscale::no_serialization,
		__wi_main_split,
		__wi_main_process
	>;


// -- entry point --

int main(int argc, char* argv[]) {
    return allscale::runtime::main_wrapper<__wi_main_work>();
}
