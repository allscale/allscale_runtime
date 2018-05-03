
#ifndef ALLSCALE_DETAIL_TREETURE_TASK_HPP
#define ALLSCALE_DETAIL_TREETURE_TASK_HPP

#include <allscale/treeture_fwd.hpp>

#include <hpx/lcos/detail/future_data.hpp>
#include <hpx/runtime/serialization/serialize.hpp>

#include <array>

namespace allscale
{
    namespace detail
    {
        template <typename T>
        struct treeture_task : hpx::lcos::detail::future_data<T>
        {
        private:
            typedef hpx::lcos::detail::future_data<T> base_type;

        public:
            typedef typename base_type::init_no_addref init_no_addref;
            treeture_task()
              : base_type()
            {
                children_[0] = treeture<void>();
                children_[1] = treeture<void>();
            }

            treeture_task(init_no_addref no_addref)
              : base_type(no_addref)
            {
                children_[0] = treeture<void>();
                children_[1] = treeture<void>();
            }

            template <typename Target>
            treeture_task(Target&& data, init_no_addref no_addref)
              : base_type(no_addref, std::forward<Target>(data))
            {
                children_[0] = treeture<void>();
                children_[1] = treeture<void>();
            }

            treeture<void> get_left_child() const
            {
                std::lock_guard<typename base_type::mutex_type> l(this->mtx_);
//                 HPX_ASSERT(children_[0]);
                return children_[0];
            }

            treeture<void> get_right_child() const
            {
                std::lock_guard<typename base_type::mutex_type> l(this->mtx_);
//                 HPX_ASSERT(children_[1]);
                return children_[1];
            }

            void set_children(treeture<void> left, treeture<void> right)
            {
                std::lock_guard<typename base_type::mutex_type> l(this->mtx_);
                children_[0] = std::move(left);
                children_[1] = std::move(right);
            }

            void set_left_child(treeture<void> child)
            {
                std::lock_guard<typename base_type::mutex_type> l(this->mtx_);
//                 HPX_ASSERT(child);
                children_[0] = std::move(child);
            }

            void set_right_child(treeture<void> child)
            {
                std::lock_guard<typename base_type::mutex_type> l(this->mtx_);
//                 HPX_ASSERT(child);
                children_[1] = std::move(child);
            }

            std::array<treeture<void>, 2> children_;
        };

        template <typename T>
        treeture<void> treeture_lco<T>::get_left_child()
        {
            return shared_state_->get_left_child();
        }

        template <typename T>
        treeture<void> treeture_lco<T>::get_right_child()
        {
            return shared_state_->get_right_child();
        }

        template <typename T>
        void treeture_lco<T>::set_children(treeture<void> left, treeture<void> right)
        {
            shared_state_->set_children(std::move(left), std::move(right));
        }

        template <typename T>
        void treeture_lco<T>::set_left_child(treeture<void> child)
        {
            shared_state_->set_left_child(std::move(child));
        }

        template <typename T>
        void treeture_lco<T>::set_right_child(treeture<void> child)
        {
            shared_state_->set_right_child(std::move(child));
        }
    }
}

#endif
