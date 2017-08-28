
#ifndef ALLSCALE_RESILIENCE_HPP
#define ALLSCALE_RESILIENCE_HPP

#include <allscale/this_work_item.hpp>

#include <functional>
#include <memory>

namespace allscale {
    namespace components {
        struct resilience;
    }
    struct resilience
    {
        static components::resilience* run(std::size_t rank);
        static void stop();
        static components::resilience *get_ptr();
        static components::resilience &get();
        static hpx::id_type get_protectee();
        private:
            static std::size_t rank_;
            resilience(std::uint64_t rank);
            std::shared_ptr<components::resilience> component_;
    };
}

#endif
