
#include <allscale/task_id.hpp>
#include <allscale/this_work_item.hpp>
#include <allscale/work_item.hpp>

namespace allscale {

    task_id task_id::parent() const
    {
        if (is_root())
        {
            auto this_wi = this_work_item::get();
            if (this_wi == nullptr)
            {
                task_id res;
                res.locality_id = 0;
                res.id = 0;
                res.path = task_path::root();
                return res;
            }
            return this_wi->id();
        }
        return {locality_id, id, path.getParentPath(), nullptr};
    }

    task_id task_id::create_root()
    {
        static std::atomic<std::uint64_t> id(1);
        task_id res;
        res.locality_id = hpx::get_locality_id();
        res.id = id++;
        res.path = task_path::root();

        return res;
    }

    task_id task_id::create_child()
    {
        auto cur_wi = this_work_item::get();

        if(!cur_wi) return create_root();

        auto pos = cur_wi->num_children++;

        switch (pos)
        {
            case 0:
                return cur_wi->id().left_child();
                break;
            case 1:
                return cur_wi->id().right_child();
                break;
            default:
                HPX_ASSERT(false);
        }
    }

    std::string to_string(task_id const& id)
    {
        std::stringstream ss;

        ss << id;

        return ss.str();
    }
}
