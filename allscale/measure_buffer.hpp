
#ifndef ALLSCALE_MEASURE_BUFFER_HPP
#define ALLSCALE_MEASURE_BUFFER_HPP

#include <array>
#include <chrono>

namespace allscale
{
    template <typename T, std::size_t Size>
    struct measure_buffer
    {
        using time_point = std::chrono::high_resolution_clock::time_point;

        std::array<T, Size> data_;
        std::array<time_point, Size> times_;

        std::size_t counter_ = 0;
        std::size_t next_ = 0;

    public:
        template <typename U>
        void push(U&& u, time_point const& now)
        {
            data_[next_] = std::forward<U>(u);
            times_[next_] = now;
            next_ = (next_ + 1) % Size;
            counter_++;
        }

        T const& oldest_data() const
        {
            return data_[oldest_position()];
        }

        time_point const& oldest_time() const
        {
            return times_[oldest_position()];
        }

    private:
        std::size_t oldest_position() const
        {
            return (counter_ < Size) ? 0 : next_;
        }
    };
}

#endif
