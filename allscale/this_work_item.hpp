
#ifndef ALLSCALE_THIS_WORK_ITEM_HPP
#define ALLSCALE_THIS_WORK_ITEM_HPP

#include <allscale/treeture.hpp>

#include <cstddef>
#include <list>
#include <string>

#include <memory>

#include <hpx/include/serialization.hpp>
#include <hpx/runtime/serialization/list.hpp>

namespace allscale {
    struct work_item;
}

namespace allscale { namespace this_work_item {
    struct id;

    id& get_id();
    void set_id(id const& id);

    struct id
    {
        id(std::size_t i)
          : id_(1, i)
          , next_id_(0)
        {
        }

        id();

        void set(id const& id, hpx::id_type const& tres = hpx::id_type());

        std::string name() const;
        std::size_t last() const;

        std::size_t hash() const;

        id parent() const;

        template <typename Result>
        treeture<Result> get_treeture() const
        {
            if (tres_)
                return treeture<Result>(hpx::make_ready_future(tres_));
            else
                return treeture<Result>();
        }

        explicit operator bool() const
        {
            return !id_.empty();
        }

    private:
        friend bool operator==(id const& lhs, id const& rhs);
        friend bool operator!=(id const& lhs, id const& rhs);
        friend bool operator<=(id const& lhs, id const& rhs);
        friend bool operator>=(id const& lhs, id const& rhs);
        friend bool operator<(id const& lhs, id const& rhs);
        friend bool operator>(id const& lhs, id const& rhs);

        std::list<std::size_t> id_;
        std::size_t next_id_;
        hpx::id_type tres_;

        friend class hpx::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & id_;
            ar & next_id_;
            ar & tres_;
        }
    };
}}

namespace std
{
    template <>
    struct hash<allscale::this_work_item::id>
    {
        std::size_t operator()(allscale::this_work_item::id const& id) const
        {
            return id.hash();
        }
    };
}

#endif
