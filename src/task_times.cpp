#include <allscale/task_times.hpp>

namespace allscale
{
    constexpr std::size_t TRACKING_LEVEL = 14;

    task_times::task_times()
      : times(1<<TRACKING_LEVEL, 0.0f)
    {}

    void task_times::add(task_id const& id, time_t const& time)
    {
        add(id.path, time);
    }

    void task_times::add(task_id::task_path const& path, time_t const& time)
    {
        // On the right level, just keep a record...
        if (path.getLength() == TRACKING_LEVEL)
        {
            times[path.getPath()] += (time.count() * 1e-9f);
            return;
        }

        // If too coarse grain, split
        if (path.getLength() < TRACKING_LEVEL)
        {
            auto f = time/2;
            add(path.getLeftChildPath(), f);
            add(path.getRightChildPath(), time-f);
            return;
        }

        // if too fine, coarsen
        add(path.getParentPath(), time);
    }

    float task_times::get_time(task_id::task_path const& path) const
    {
        // on the right level, return the recorded time
        if (path.getLength() == TRACKING_LEVEL)
        {
            return times[path.getPath()];
        }

        // If too coarse grain, sum up partial times
        if (path.getLength() < TRACKING_LEVEL)
        {
            return get_time(path.getLeftChildPath()) + get_time(path.getRightChildPath());
        }

        // if too fine, estimate half of our parent
        return get_time(path.getParentPath()) / 2.f;
    }

    task_times& task_times::operator+=(task_times const& other)
    {
        for (std::size_t i = 0; i < times.size(); ++i)
        {
            times[i] += other.times[i];
        }

        return *this;
    }

    task_times task_times::operator-(task_times const& other)
    {
        task_times res(*this);
        for (std::size_t i = 0; i < times.size(); ++i)
        {
            res.times[i] -= other.times[i];
        }

        return res;
    }

    task_times task_times::operator/(float f)
    {
        task_times res(*this);
        for (std::size_t i = 0; i < times.size(); ++i)
        {
            res.times[i] /= f;
        }

        return res;
    }
}
