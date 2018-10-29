
#ifndef ALLSCALE_TASK_TIMES_HPP
#define ALLSCALE_TASK_TIMES_HPP

#include <allscale/task_id.hpp>

#include <chrono>
#include <iostream>
#include <vector>

namespace allscale {
    struct task_times
    {
        using time_t = std::chrono::nanoseconds;

        std::vector<float> times;
    public:

        task_times();

        void add(task_id const& id, time_t const& time);
        void add(task_id::task_path const& path, time_t const& time);

        float get_time(task_id::task_path const& path) const;

        task_times& operator+=(task_times const& other);
        task_times operator-(task_times const& other);
        task_times operator/(float f);

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & times;
        }

        friend std::ostream& operator<<(std::ostream& os, task_times const& times)
        {
            os << '[';
            for (std::size_t i = 0; i != times.times.size()-1; ++i)
            {
                os << times.times[i] << ", ";
            }
            os << times.times.back() << ']';

            return os;
        }
    };
}

#endif
