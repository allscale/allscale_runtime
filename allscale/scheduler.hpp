
#ifndef ALLSCALE_SCHEDULER_HPP
#define ALLSCALE_SCHEDULER_HPP

#include <hpx/config.hpp>
#include <allscale/optimizer.hpp>

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

    struct work_item;

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

        static HPX_EXPORT std::size_t rank();
        static HPX_EXPORT hpx::future<void> toggle_node(std::size_t locality_id);
        static HPX_EXPORT hpx::future<void> set_policy(std::string policy);
        static HPX_EXPORT hpx::future<void> set_speed_exponent(float exp);
        static HPX_EXPORT hpx::future<void> set_efficiency_exponent(float exp);
        static HPX_EXPORT hpx::future<void> set_power_exponent(float exp);
        static HPX_EXPORT hpx::util::tuple<float, float, float> get_optimizer_exponents();
        static HPX_EXPORT std::string policy();

        static HPX_EXPORT void update_policy(task_times const& times, std::vector<bool> mask, std::uint64_t frequency);
        static void apply_new_mapping(const std::vector<std::size_t> &new_mapping);

        static HPX_EXPORT void schedule(work_item&& work);
        static HPX_EXPORT components::scheduler* run(std::size_t rank);
        static HPX_EXPORT void stop();
        static components::scheduler* get_ptr();

        static components::scheduler & get();

        static bool active();

        static void toggle_active(bool toggle = true);

    private:

        typedef hpx::lcos::local::spinlock mutex_type;
        static HPX_EXPORT void partition_resources(hpx::resource::partitioner& rp);

        static mutex_type active_mtx_;
        static bool active_;

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
        std::vector<std::string> cfg = {
            "hpx.run_hpx_main!=1",
            "hpx.commandline.allow_unknown!=1"
        };
        hpx::resource::partitioner rp(desc, argc, argv, cfg);
        partition_resources(rp);
    }

    void scheduler::setup_resources(
        int(*hpx_main)(boost::program_options::variables_map &),
        boost::program_options::options_description const & desc,
        int argc, char** argv)
    {
        std::vector<std::string> cfg = {
            "hpx.run_hpx_main!=1",
            "hpx.commandline.allow_unknown!=1"
        };
        if (argc == 0)
        {
            static const char* argv_[] = {"allscale"};
            argv = const_cast<char **>(argv_);
            argc = 1;
        }
        hpx::resource::partitioner rp(hpx_main, desc, argc, argv, std::move(cfg));
        partition_resources(rp);
    }
}

#endif
