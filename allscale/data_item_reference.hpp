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
        {
            typedef void(*release_function_type)(hpx::naming::gid_type const&);

            explicit id_holder(release_function_type release_function)
              : release_function_(release_function)
            {}

            ~id_holder()
            {
                HPX_ASSERT(release_function_);
                HPX_ASSERT(this->get_id());
                release_function_(this->get_id().get_gid());
            }

            release_function_type release_function_;
        };
    }

    template<typename DataItemType>
    class data_item_reference
    {
    public:
        typedef DataItemType data_item_type;
        typedef typename DataItemType::shared_data_type shared_data_type;
        typedef typename DataItemType::fragment_type fragment_type;

        // NOTE: data_item_reference currently implements a single ownership
        // semantic as follows:
        // Only the first data_item_reference that was created has ownership
        // Ownership is transferred with move and copy construction or assignment
        // Serialization does not propagate the ownership.
        //
        // CAUTION: This requires the user to ensure that the owning
        // data_item_reference stays alive long enough!
        //
        // This is implemented by using the (global) reference counting mechanism
        // of HPX components.
        // If true global reference counting is required in the end, the
        // make_unmanaged call in the save function needs to be removed.

        data_item_reference()
          : fragment(nullptr)
        {}

        template <typename...Args>
        explicit data_item_reference(hpx::future<hpx::id_type> id)
          : fragment(nullptr)
          , id_(id.get())
        {
            HPX_ASSERT(id_.get_management_type() == hpx::id_type::managed);
        }

        data_item_reference(data_item_reference const& other)
          : fragment(other.fragment)
          , id_(other.id_)
        {
        }

        data_item_reference(data_item_reference&& other) noexcept
          : fragment(other.fragment)
          , id_(std::move(other.id_))
        {
        }

        data_item_reference& operator=(data_item_reference const& other)
        {
            fragment = other.fragment;
            id_ = other.id_;

            return *this;
        }


        data_item_reference& operator=(data_item_reference&& other) noexcept
        {
            fragment = other.fragment;
            id_ = std::move(other.id_);

            return *this;
        }

        template <typename Archive>
        void load(Archive & ar, unsigned)
        {
            hpx::naming::gid_type id;
            ar & id;
            id_ = hpx::id_type(id, hpx::id_type::unmanaged);
        }

        template <typename Archive>
        void save(Archive & ar, unsigned) const
        {
            hpx::naming::gid_type id = id_.get_gid();
            ar & id;
        }
        HPX_SERIALIZATION_SPLIT_MEMBER()

        hpx::naming::gid_type id() const
        {
            return id_.get_gid();
        }

        mutable fragment_type *__restrict__ fragment;

    private:
        hpx::id_type id_;
    };
}

#endif
