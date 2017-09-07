
#ifndef ALLSCALE_SCHEDULER_HPP
#define ALLSCALE_SCHEDULER_HPP

#include <hpx/config.hpp>
#include <allscale/work_item.hpp>

#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/resource_partitioner.hpp>

#include <boost/program_options.hpp>

#include <memory>

namespace allscale
{
    namespace components
    {
        struct scheduler;
    }

    struct scheduler
    {
        // This function needs to be called before hpx::init.
        static inline void setup_resources(int argc = 0, char** argv = nullptr);
        static inline void setup_resources(
                boost::program_options::options_description const &,
                int argc, char** argv);
        static inline void setup_resources(
                int(*hpx_main)(boost::program_options::variables_map &),
                boost::program_options::options_description const &,
                int argc, char** argv);

        scheduler() { HPX_ASSERT(false); }
        scheduler(std::size_t rank);

        static void schedule(work_item work);
        static components::scheduler* run(std::size_t rank);
        static void stop();
        static components::scheduler* get_ptr();

    private:
        static std::size_t rank_;
        static components::scheduler & get();

        typedef hpx::lcos::local::spinlock mutex_type;
        mutex_type mtx_;
        static void partition_resources(hpx::resource::partitioner& rp);

        std::shared_ptr<components::scheduler> component_;
    };

    void scheduler::setup_resources(int argc, char** argv)
    {
        if (argc == 0)
        {
            static const char* argv_[] = {"allscale"};
            argv = const_cast<char **>(argv_);
            argc = 1;
        }
        boost::program_options::options_description desc;
        setup_resources(desc, argc, argv);
    }

    void scheduler::setup_resources(
        boost::program_options::options_description const & desc,
        int argc, char** argv)
    {
        hpx::resource::partitioner rp(desc, argc, argv);
        partition_resources(rp);
    }

    void scheduler::setup_resources(
        int(*hpx_main)(boost::program_options::variables_map &),
        boost::program_options::options_description const & desc,
        int argc, char** argv)
    {
        std::vector<std::string> cfg;
        if (argc == 0)
        {
            static const char* argv_[] = {"allscale"};
            argv = const_cast<char **>(argv_);
            argc = 1;
        }
        hpx::resource::partitioner rp(hpx_main, desc, argc, argv, cfg);
        partition_resources(rp);
    }
}

#endif
