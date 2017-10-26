
#include <allscale/runtime.hpp>
#include <allscale/api/user/data/grid.h>

using STREAM_TYPE = double;

using vector = allscale::api::user::data::Grid<STREAM_TYPE, 1>;
using region_type = vector::region_type;

ALLSCALE_REGISTER_TREETURE_TYPE(int)
REGISTER_DATAITEMSERVER_DECLARATION(vector);
REGISTER_DATAITEMSERVER(vector);

////////////////////////////////////////////////////////////////////////////////
// Boilerplate...
template <typename Algorithm>
struct stream_name
{
    static const char* name() { return typeid(Algorithm).name(); }
};

struct stream_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        return end - begin > 1000;
    }

};

template <typename WorkItem>
struct stream_split
{
    template <typename T>
    static hpx::future<void> execute(hpx::util::tuple<T, T> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto mid = begin + (end - begin) / 2;

        return hpx::when_all(
            allscale::spawn<WorkItem>(begin, mid).get_future(),
            allscale::spawn<WorkItem>(mid, end).get_future()
        );
    }

    template <typename T, typename D>
    static hpx::future<void> execute(hpx::util::tuple<T, T, D> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto& a1 = hpx::util::get<2>(c);

        auto mid = begin + (end - begin) / 2;

        return hpx::when_all(
            allscale::spawn<WorkItem>(begin, mid, a1).get_future(),
            allscale::spawn<WorkItem>(mid, end, a1).get_future()
        );
    }

    template <typename T, typename D1, typename D2>
    static hpx::future<void> execute(hpx::util::tuple<T, T, D1, D2> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto& a1 = hpx::util::get<2>(c);
        auto& a2 = hpx::util::get<3>(c);

        auto mid = begin + (end - begin) / 2;

        return hpx::when_all(
            allscale::spawn<WorkItem>(begin, mid, a1, a2).get_future(),
            allscale::spawn<WorkItem>(mid, end, a1, a2).get_future()
        );
    }

    template <typename T, typename D1, typename D2, typename D3>
    static hpx::future<void> execute(hpx::util::tuple<T, T, D1, D2, D3> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto& a1 = hpx::util::get<2>(c);
        auto& a2 = hpx::util::get<3>(c);
        auto& a3 = hpx::util::get<4>(c);

        auto mid = begin + (end - begin) / 2;

        return hpx::when_all(
            allscale::spawn<WorkItem>(begin, mid, a1, a2, a3).get_future(),
            allscale::spawn<WorkItem>(mid, end, a1, a2, a3).get_future()
        );
    }

    template <typename T, typename D1, typename D2, typename D3, typename D4>
    static hpx::future<void> execute(hpx::util::tuple<T, T, D1, D2, D3, D4> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto& a1 = hpx::util::get<2>(c);
        auto& a2 = hpx::util::get<3>(c);
        auto& a3 = hpx::util::get<4>(c);
        auto& a4 = hpx::util::get<5>(c);

        auto mid = begin + (end - begin) / 2;

        return hpx::when_all(
            allscale::spawn<WorkItem>(begin, mid, a1, a2, a3, a4).get_future(),
            allscale::spawn<WorkItem>(mid, end, a1, a2, a3, a4).get_future()
        );
    }

    static constexpr bool valid = true;
};

template <typename Work>
struct stream_process
{
    template <typename T>
    static hpx::util::unused_type execute(hpx::util::tuple<T, T> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        Work()(begin, end);

        return hpx::util::unused;
    }

    template <typename T, typename D>
    static hpx::util::unused_type execute(hpx::util::tuple<T, T, D> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto& a1 = hpx::util::get<2>(c);

        Work()(begin, end, a1);

        return hpx::util::unused;
    }

    template <typename T, typename D1, typename D2>
    static hpx::util::unused_type execute(hpx::util::tuple<T, T, D1, D2> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto& a1 = hpx::util::get<2>(c);
        auto& a2 = hpx::util::get<3>(c);

        Work()(begin, end, a1, a2);

        return hpx::util::unused;
    }

    template <typename T, typename D1, typename D2, typename D3>
    static hpx::util::unused_type execute(hpx::util::tuple<T, T, D1, D2, D3> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto& a1 = hpx::util::get<2>(c);
        auto& a2 = hpx::util::get<3>(c);
        auto& a3 = hpx::util::get<4>(c);

        Work()(begin, end, a1, a2, a3);

        return hpx::util::unused;
    }

    template <typename T, typename D1, typename D2, typename D3, typename D4>
    static hpx::util::unused_type execute(hpx::util::tuple<T, T, D1, D2, D3, D4> const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto& a1 = hpx::util::get<2>(c);
        auto& a2 = hpx::util::get<3>(c);
        auto& a3 = hpx::util::get<4>(c);
        auto& a4 = hpx::util::get<5>(c);

        Work()(begin, end, a1, a2, a3, a4);

        return hpx::util::unused;
    }

    template <typename Closure>
    static auto get_requirements(Closure const& c)
    {
        return Work::requirements(c);
    }

    static constexpr bool valid = true;
};

///////////////////////////////////////////////////////////////////////////////
double mysecond()
{
    return hpx::util::high_resolution_clock::now() * 1e-9;
}

int checktick()
{
    static const std::size_t M = 20;
    int minDelta, Delta;
    double t1, t2, timesfound[M];

    // Collect a sequence of M unique time values from the system.
    for (std::size_t i = 0; i < M; i++) {
        t1 = mysecond();
        while( ((t2=mysecond()) - t1) < 1.0E-6 )
            ;
        timesfound[i] = t1 = t2;
    }

    // Determine the minimum difference between these M values.
    // This result will be our estimate (in microseconds) for the
    // clock granularity.
    minDelta = 1000000;
    for (std::size_t i = 1; i < M; i++) {
        Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
        minDelta = (std::min)(minDelta, (std::max)(Delta,0));
    }

    return(minDelta);
}

template <typename Work>
struct stream
  : allscale::work_item_description<
        void,
        stream_name<Work>,
        allscale::do_serialization,
        stream_split<stream<Work>>,
        stream_process<Work>,
        stream_can_split>
{};

struct init
{
    template <typename Closure>
    static auto
    requirements(Closure const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto const& ref = hpx::util::get<2>(c);
        region_type r(begin, end);
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                ref,
                r,
                allscale::access_mode::ReadWrite
            )
        );
    }

    void operator()(int begin, int end,
        allscale::data_item_reference<vector> const& ref, STREAM_TYPE init) const
    {
        auto data = allscale::data_item_manager::get(ref);
        for (int i = begin; i != end; ++i)
        {
            data[i] = init;
        }
    }
};
using init_work = stream<init>;

struct copy
{
    template <typename Closure>
    static auto
    requirements(Closure const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto const& ref1 = hpx::util::get<2>(c);
        auto const& ref2 = hpx::util::get<3>(c);
        region_type r(begin, end);
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                ref1,
                r,
                allscale::access_mode::ReadWrite
            ),
            allscale::createDataItemRequirement(
                ref2,
                r,
                allscale::access_mode::ReadOnly
            )
        );
    }

    void operator()(int begin, int end,
        allscale::data_item_reference<vector> const& ref_a,
        allscale::data_item_reference<vector> const& ref_b) const
    {
        auto a = allscale::data_item_manager::get(ref_a);
        auto b = allscale::data_item_manager::get(ref_b);
        for (int i = begin; i != end; ++i)
        {
            a[i] = b[i];
        }
    }
};
using copy_work = stream<copy>;

struct scale
{
    template <typename Closure>
    static auto
    requirements(Closure const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto const& ref1 = hpx::util::get<2>(c);
        auto const& ref2 = hpx::util::get<3>(c);
        region_type r(begin, end);
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                ref1,
                r,
                allscale::access_mode::ReadWrite
            ),
            allscale::createDataItemRequirement(
                ref2,
                r,
                allscale::access_mode::ReadOnly
            )
        );
    }

    void operator()(int begin, int end,
        allscale::data_item_reference<vector> const& ref_a,
        allscale::data_item_reference<vector> const& ref_b,
        STREAM_TYPE scalar) const
    {
        auto a = allscale::data_item_manager::get(ref_a);
        auto b = allscale::data_item_manager::get(ref_b);
        for (int i = begin; i != end; ++i)
        {
            a[i] = scalar * b[i];
        }
    }
};
using scale_work = stream<scale>;

struct add
{
    template <typename Closure>
    static auto
    requirements(Closure const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto const& ref1 = hpx::util::get<2>(c);
        auto const& ref2 = hpx::util::get<3>(c);
        auto const& ref3 = hpx::util::get<4>(c);
        region_type r(begin, end);
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                ref1,
                r,
                allscale::access_mode::ReadWrite
            ),
            allscale::createDataItemRequirement(
                ref2,
                r,
                allscale::access_mode::ReadOnly
            ),
            allscale::createDataItemRequirement(
                ref3,
                r,
                allscale::access_mode::ReadOnly
            )
        );
    }

    void operator()(int begin, int end,
        allscale::data_item_reference<vector> const& ref_a,
        allscale::data_item_reference<vector> const& ref_b,
        allscale::data_item_reference<vector> const& ref_c) const
    {
        auto a = allscale::data_item_manager::get(ref_a);
        auto b = allscale::data_item_manager::get(ref_b);
        auto c = allscale::data_item_manager::get(ref_c);
        for (int i = begin; i != end; ++i)
        {
            a[i] = b[i] + c[i];
        }
    }
};
using add_work = stream<add>;

struct triad
{
    template <typename Closure>
    static auto
    requirements(Closure const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto const& ref1 = hpx::util::get<2>(c);
        auto const& ref2 = hpx::util::get<3>(c);
        auto const& ref3 = hpx::util::get<4>(c);
        region_type r(begin, end);
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                ref1,
                r,
                allscale::access_mode::ReadWrite
            ),
            allscale::createDataItemRequirement(
                ref2,
                r,
                allscale::access_mode::ReadOnly
            ),
            allscale::createDataItemRequirement(
                ref3,
                r,
                allscale::access_mode::ReadOnly
            )
        );
    }

    void operator()(int begin, int end,
        allscale::data_item_reference<vector> const& ref_a,
        allscale::data_item_reference<vector> const& ref_b,
        allscale::data_item_reference<vector> const& ref_c,
        STREAM_TYPE scalar) const
    {
        auto a = allscale::data_item_manager::get(ref_a);
        auto b = allscale::data_item_manager::get(ref_b);
        auto c = allscale::data_item_manager::get(ref_c);
        for (int i = begin; i != end; ++i)
        {
            a[i] = b[i] + scalar * c[i];
        }
    }
};
using triad_work = stream<triad>;

struct check_results
{
    template <typename Closure>
    static auto
    requirements(Closure const& c)
    {
        auto begin = hpx::util::get<0>(c);
        auto end = hpx::util::get<1>(c);

        auto const& ref1 = hpx::util::get<2>(c);
        auto const& ref2 = hpx::util::get<3>(c);
        auto const& ref3 = hpx::util::get<4>(c);
        region_type r(begin, end);
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                ref1,
                r,
                allscale::access_mode::ReadOnly
            ),
            allscale::createDataItemRequirement(
                ref2,
                r,
                allscale::access_mode::ReadOnly
            ),
            allscale::createDataItemRequirement(
                ref3,
                r,
                allscale::access_mode::ReadOnly
            )
        );
    }

    void operator()(int begin, int end,
        allscale::data_item_reference<vector> const& ref_a,
        allscale::data_item_reference<vector> const& ref_b,
        allscale::data_item_reference<vector> const& ref_c,
        std::size_t iterations) const
    {
        auto a = allscale::data_item_manager::get(ref_a);
        auto b = allscale::data_item_manager::get(ref_b);
        auto c = allscale::data_item_manager::get(ref_c);

        STREAM_TYPE aj,bj,cj,scalar;
        STREAM_TYPE aSumErr,bSumErr,cSumErr;
        STREAM_TYPE aAvgErr,bAvgErr,cAvgErr;
        double epsilon;
        int ierr,err;

        /* reproduce initialization */
        aj = 1.0;
        bj = 2.0;
        cj = 0.0;
        /* a[] is modified during timing check */
//         aj = 2.0E0 * aj;
        /* now execute timing loop */
        scalar = 3.0;
        for (std::size_t k=0; k<iterations; k++)
            {
                cj = aj;
                bj = scalar*cj;
                cj = aj+bj;
                aj = bj+scalar*cj;
            }

        /* accumulate deltas between observed and expected results */
        aSumErr = 0.0;
        bSumErr = 0.0;
        cSumErr = 0.0;
        for (int j=begin; j<end; j++) {
            aSumErr += std::abs(a[j] - aj);
            bSumErr += std::abs(b[j] - bj);
            cSumErr += std::abs(c[j] - cj);
        }

        aAvgErr = aSumErr / (STREAM_TYPE) (end - begin);
        bAvgErr = bSumErr / (STREAM_TYPE) (end - begin);
        cAvgErr = cSumErr / (STREAM_TYPE) (end - begin);

        if (sizeof(STREAM_TYPE) == 4) {
            epsilon = 1.e-6;
        }
        else if (sizeof(STREAM_TYPE) == 8) {
            epsilon = 1.e-13;
        }
        else {
            printf("WEIRD: sizeof(STREAM_TYPE) = %zu\n", sizeof(STREAM_TYPE));
            epsilon = 1.e-6;
        }

        err = 0;
        if (std::abs(aAvgErr/aj) > epsilon) {
            err++;
            printf ("Failed Validation on array a[], AvgRelAbsErr > epsilon (%e)\n",
                epsilon);
            printf ("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n",
                aj,aAvgErr,std::abs(aAvgErr)/aj);
            ierr = 0;
            for (int j=begin; j<end; j++) {
                if (std::abs(a[j]/aj-1.0) > epsilon) {
                    ierr++;
#ifdef VERBOSE
                    if (ierr < 10) {
                        printf("         array a: index: %ld, expected: %e, "
                            "observed: %e, relative error: %e\n",
                            j,aj,a[j],std::abs((aj-a[j])/aAvgErr));
                    }
#endif
                }
            }
        }
        if (std::abs(bAvgErr/bj) > epsilon) {
            err++;
            printf ("Failed Validation on array b[], AvgRelAbsErr > epsilon (%e)\n",
                epsilon);
            printf ("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n",
                bj,bAvgErr,std::abs(bAvgErr)/bj);
            printf ("     AvgRelAbsErr > Epsilon (%e)\n",epsilon);
            ierr = 0;
            for (int j=begin; j<end; j++) {
                if (std::abs(b[j]/bj-1.0) > epsilon) {
                    ierr++;
#ifdef VERBOSE
                    if (ierr < 10) {
                        printf("         array b: index: %ld, expected: %e, "
                            "observed: %e, relative error: %e\n",
                            j,bj,b[j],std::abs((bj-b[j])/bAvgErr));
                    }
#endif
                }
            }
            printf("     For array b[], %d errors were found.\n",ierr);
        }
        if (std::abs(cAvgErr/cj) > epsilon) {
            err++;
            printf ("Failed Validation on array c[], AvgRelAbsErr > epsilon (%e)\n",
                epsilon);
            printf ("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n",
                cj,cAvgErr,std::abs(cAvgErr)/cj);
            printf ("     AvgRelAbsErr > Epsilon (%e)\n",epsilon);
            ierr = 0;
            for (int j=begin; j<end; j++) {
                if (std::abs(c[j]/cj-1.0) > epsilon) {
                    ierr++;
#ifdef VERBOSE
                    if (ierr < 10) {
                        printf("         array c: index: %ld, expected: %e, "
                            "observed: %e, relative error: %e\n",
                            j,cj,c[j],std::abs((cj-c[j])/cAvgErr));
                    }
#endif
                }
            }
            printf("     For array c[], %d errors were found.\n",ierr);
        }
        if (err == 0) {
            printf ("Solution Validates: avg error less than %e on all three arrays\n",
                epsilon);
        }
#ifdef VERBOSE
        printf ("Results Validation Verbose Results: \n");
        printf ("    Expected a(1), b(1), c(1): %f %f %f \n",aj,bj,cj);
        printf ("    Observed a(1), b(1), c(1): %f %f %f \n",a[1],b[1],c[1]);
        printf ("    Rel Errors on a, b, c:     %e %e %e \n",std::abs(aAvgErr/aj),
            std::abs(bAvgErr/bj),std::abs(cAvgErr/cj));
#endif
    }
};


using check_results_work = stream<check_results>;

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
    static allscale::treeture<int> execute(hpx::util::tuple<int, char**> const& clos)
    {
        int argc = hpx::util::get<0>(clos);
        char **argv = hpx::util::get<1>(clos);

        int size = 1024;
        if (argc == 2)
        {
            size = std::atoi(argv[1]);
        }
        std::size_t iterations = 10;
        if (argc == 3)
        {
            size = std::atoi(argv[2]);
        }

        std::cout
            << "-------------------------------------------------------------\n"
            << "Modified STREAM bechmark based on AllScale                   \n"
            << "-------------------------------------------------------------\n"
            << "This system uses " << sizeof(STREAM_TYPE)
                << " bytes per array element.\n"
            << "-------------------------------------------------------------\n"
            << "Array size = " << size << " (elements)                       \n"
            << "Memory per array = "
                << sizeof(STREAM_TYPE) * (size / 1024. / 1024.) << " MiB "
            << "(= "
                <<  sizeof(STREAM_TYPE) * (size / 1024. / 1024. / 1024.)
                << " GiB).\n"
            << "Each kernel will be executed " << iterations << " times.\n"
            << " The *best* time for each kernel (excluding the first iteration)\n"
            << " will be used to compute the reported bandwidth.\n"
            << "-------------------------------------------------------------\n"
            << "Number of Threads requested = "
                << hpx::get_os_thread_count() << "\n"
            << "-------------------------------------------------------------\n"
            ;

        allscale::data_item_reference<vector> a
            = allscale::data_item_manager::create<vector>(size);
        allscale::data_item_reference<vector> b
            = allscale::data_item_manager::create<vector>(size);
        allscale::data_item_reference<vector> c
            = allscale::data_item_manager::create<vector>(size);

        double time_total = mysecond();

        // Initialize arrays
        hpx::when_all(
        allscale::spawn_first<init_work>(0, size, a, 1.0).get_future(),
        allscale::spawn_first<init_work>(0, size, b, 1.0).get_future(),
        allscale::spawn_first<init_work>(0, size, c, 1.0).get_future()).get();

        // Check clock ticks ...
        double t = mysecond();
//         hpx::parallel::transform(
//             policy, a.begin(), a.end(), a.begin(),
//             multiply_step<STREAM_TYPE>(2.0));
        t = 1.0E6 * (mysecond() - t);

        // Get initial value for system clock.
        int quantum = checktick();
        if(quantum >= 1)
        {
            std::cout
                << "Your clock granularity/precision appears to be " << quantum
                << " microseconds.\n"
                ;
        }
        else
        {
            std::cout
                << "Your clock granularity appears to be less than one microsecond.\n"
                ;
            quantum = 1;
        }

        std::cout
            << "Each test below will take on the order"
            << " of " << (int) t << " microseconds.\n"
            << "   (= " << (int) (t/quantum) << " clock ticks)\n"
            << "Increase the size of the arrays if this shows that\n"
            << "you are not getting at least 20 clock ticks per test.\n"
            << "-------------------------------------------------------------\n"
            ;

        std::cout
            << "WARNING -- The above is only a rough guideline.\n"
            << "For best results, please be sure you know the\n"
            << "precision of your system timer.\n"
            << "-------------------------------------------------------------\n"
            ;

        std::vector<std::vector<double> > timing(4, std::vector<double>(iterations));

        double scalar = 3.0;
        for (std::size_t iteration = 0; iteration != iterations; ++iteration)
        {
            // Copy: c = a
            timing[0][iteration] = mysecond();
            allscale::spawn_first<copy_work>(0, size, c, a).wait();
            timing[0][iteration] = mysecond() - timing[0][iteration];

            // Scale: b = scalar * c
            timing[1][iteration] = mysecond();
            allscale::spawn_first<scale_work>(0, size, b, c, scalar).wait();
            timing[1][iteration] = mysecond() - timing[1][iteration];

            // Add: c = a + b
            timing[2][iteration] = mysecond();
            allscale::spawn_first<add_work>(0, size, c, a, b).wait();
            timing[2][iteration] = mysecond() - timing[2][iteration];

            // Triad: a = b + scalar * c
            timing[3][iteration] = mysecond();
            allscale::spawn_first<triad_work>(0, size, a, b, c, scalar).wait();
            timing[3][iteration] = mysecond() - timing[3][iteration];
        }

        // Check Results ...
        allscale::spawn_first<check_results_work>(0, 100, a, b, c, iterations).wait();

        allscale::data_item_manager::destroy(a);
        allscale::data_item_manager::destroy(b);
        allscale::data_item_manager::destroy(c);

        time_total = mysecond() - time_total;

        /* --- SUMMARY --- */
        const char *label[4] = {
            "Copy:      ",
            "Scale:     ",
            "Add:       ",
            "Triad:     "
        };

        const double bytes[4] = {
            2 * sizeof(STREAM_TYPE) * static_cast<double>(size),
            2 * sizeof(STREAM_TYPE) * static_cast<double>(size),
            3 * sizeof(STREAM_TYPE) * static_cast<double>(size),
            3 * sizeof(STREAM_TYPE) * static_cast<double>(size)
        };

        // Note: skip first iteration
        std::vector<double> avgtime(4, 0.0);
        std::vector<double> mintime(4, (std::numeric_limits<double>::max)());
        std::vector<double> maxtime(4, 0.0);
        for(std::size_t iteration = 1; iteration != iterations; ++iteration)
        {
            for (std::size_t j=0; j<4; j++)
            {
                avgtime[j] = avgtime[j] + timing[j][iteration];
                mintime[j] = (std::min)(mintime[j], timing[j][iteration]);
                maxtime[j] = (std::max)(maxtime[j], timing[j][iteration]);
            }
        }

        printf("Function    Best Rate MB/s  Avg time     Min time     Max time\n");
        for (std::size_t j=0; j<4; j++) {
            avgtime[j] = avgtime[j]/(double)(iterations-1);

            printf("%s%12.1f  %11.6f  %11.6f  %11.6f\n", label[j],
               1.0E-06 * bytes[j]/mintime[j],
               avgtime[j],
               mintime[j],
               maxtime[j]);
        }

        std::cout
            << "\nTotal time: " << time_total
            << " (per iteration: " << time_total/iterations << ")\n";

        std::cout
            << "-------------------------------------------------------------\n"
            ;

        return allscale::make_ready_treeture(0);
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv)
{
    return allscale::runtime::main_wrapper<main_work>(argc, argv);
}
