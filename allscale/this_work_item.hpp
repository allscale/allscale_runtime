
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
    namespace detail {
        struct work_item_impl_base;
    }
    struct work_item;
}

namespace allscale { namespace this_work_item {
    struct id;

    id& get_id();
    void set_id(id const& id);

    struct id
    {
        id(std::size_t i);

        id();

//         void set(std::shared_ptr<detail::work_item_impl_base>);
        void set(detail::work_item_impl_base*);

        std::string name() const;
        std::size_t last() const;
        std::size_t depth() const;

        std::size_t hash() const;

        id parent() const;

        treeture<void> get_treeture() const;

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
        treeture<void> tres_;
//         detail::work_item_impl_base* wi_;
//         std::shared_ptr<detail::work_item_impl_base> wi_;

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
