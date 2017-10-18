#ifndef ALLSCALE_DATA_ITEM_REFERENCE_HPP
#define ALLSCALE_DATA_ITEM_REFERENCE_HPP

#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/serialization.hpp>

///////////////////////////////////////////////////////////////////////////

namespace allscale {
    namespace detail
    {
        struct id_holder
          : hpx::components::component_base<id_holder>
        {};
    }

    template<typename DataItemType>
    class data_item_reference
    {
    public:
        typedef typename DataItemType::shared_data_type shared_data_type;
        typedef typename DataItemType::fragment_type fragment_type;

        data_item_reference()
          : fragment(nullptr)
        {}

        template <typename...Args>
        data_item_reference(hpx::future<hpx::id_type> id, Args&&...args)
          : fragment(nullptr)
          , shared_data_(std::forward<Args>(args)...)
          , id_(id.get())
        {
        }

        data_item_reference(data_item_reference const& other)
          : fragment(nullptr)
          , shared_data_(other.shared_data_)
          , id_(other.id_)
        {
        }

        data_item_reference(data_item_reference&& other)
          : fragment(nullptr)
          , shared_data_(std::move(other.shared_data_))
          , id_(std::move(other.id_))
        {}

        data_item_reference& operator=(data_item_reference const& other)
        {
            fragment = nullptr;
            shared_data_ = other.shared_data_;
            id_ = other.id_;

            return *this;
        }


        data_item_reference& operator=(data_item_reference&& other)
        {
            fragment = nullptr;
            shared_data_ = std::move(other.shared_data_);
            id_ = std::move(other.id_);

            return *this;
        }

        template <typename Archive>
        void serialize(Archive & ar, unsigned)
        {
            ar & shared_data_;
            ar & id_;
        }

        hpx::id_type const& id() const
        {
            return id_;
        }

        shared_data_type const& shared_data() const
        {
            return shared_data_;
        }

        mutable fragment_type *__restrict__ fragment;

    private:
        shared_data_type shared_data_;
        hpx::id_type id_;
    };
}

#endif
