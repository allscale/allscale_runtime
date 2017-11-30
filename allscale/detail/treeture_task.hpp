
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

            treeture_task(treeture<void> parent, init_no_addref no_addref)
              : base_type(no_addref)
              , parent_(parent)
            {
                children_[0] = treeture<void>();
                children_[1] = treeture<void>();
            }

            template <typename Target>
            treeture_task(treeture<void> parent, Target&& data, init_no_addref no_addref)
              : base_type(std::forward<Target>(data), no_addref)
              , parent_(parent)
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

            treeture<void> parent_;

            std::array<treeture<void>, 2> children_;
        };

        template <typename T>
        treeture<void> treeture_lco<T>::get_parent()
        {
//             HPX_ASSERT(shared_state_->parent_);
            return shared_state_->parent_;
        }

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

        template <typename T>
        void serialize(hpx::serialization::input_archive& ar, boost::intrusive_ptr<treeture_task<T>>& treeture_task_, int)
        {
            treeture<void> parent;
//             ar >> parent;

            int state = hpx::lcos::detail::future_state::invalid;
            ar >> state;
            if (state == hpx::lcos::detail::future_state::has_value)
            {
                T value;
                ar >> value;
                treeture_task_.reset(
                    new treeture_task<T>(parent,
                        std::move(value), typename treeture_task<T>::init_no_addref()));
//             } else if (state == hpx::lcos::detail::future_state::has_exception) {
//                 boost::exception_ptr exception;
//                 ar >> exception;
//
//                 boost::intrusive_ptr<shared_state> p(
//                     new shared_state(std::move(exception), init_no_addref()), false);
//
//                 f = hpx::traits::future_access<Future>::create(std::move(p));
            } else if (state == hpx::lcos::detail::future_state::invalid) {
//                 f = Future();
            } else {
                HPX_ASSERT(false);
            }
        }

        template <typename T>
        void serialize(hpx::serialization::output_archive& ar, boost::intrusive_ptr<treeture_task<T>> treeture_task_, int)
        {
            HPX_ASSERT(treeture_task_);
//             ar << treeture_task_->parent_;

            int state = hpx::lcos::detail::future_state::invalid;
            if (ar.is_preprocessing())
            {
                if (!treeture_task_->is_ready())
                {
                    ar.await_future(treeture_task_);
                }
                else
                {
                    if (treeture_task_->has_value())
                    {
                        state = hpx::lcos::detail::future_state::has_value;
                        T const & value = *treeture_task_->get_result();
                        ar << state << value;
                    }
                    else if (treeture_task_->has_exception())
                    {
                        HPX_ASSERT(false);
//                         state = future_state::has_exception;
//                         boost::exception_ptr exception = treeture_task_->get_exception_ptr();
//                         ar << state << exception;
                    }
                    else
                    {
                        state = hpx::lcos::detail::future_state::invalid;
                        ar << state;
                    }
                }
                return;
            }

            if (treeture_task_->has_value())
            {
                state = hpx::lcos::detail::future_state::has_value;
                T const & value = *treeture_task_->get_result();
                ar << state << value;
            }
            else if (treeture_task_->has_exception())
            {
                HPX_ASSERT(false);
//                 state = future_state::has_exception;
//                 boost::exception_ptr exception = treeture_task_->get_exception_ptr();
//                 ar << state << exception;
            }
            else
            {
                state = hpx::lcos::detail::future_state::invalid;
                ar << state;
            }
        }

        inline void serialize(hpx::serialization::input_archive& ar, boost::intrusive_ptr<treeture_task<void>>& treeture_task_, int)
        {
            treeture<void> parent;
            ar >> parent;

            int state = hpx::lcos::detail::future_state::invalid;
            ar >> state;
            if (state == hpx::lcos::detail::future_state::has_value)
            {
                treeture_task_.reset(
                    new treeture_task<void>(parent, hpx::util::unused,
                        typename treeture_task<void>::init_no_addref()));
//             } else if (state == hpx::lcos::detail::future_state::has_exception) {
//                 boost::exception_ptr exception;
//                 ar >> exception;
//
//                 boost::intrusive_ptr<shared_state> p(
//                     new shared_state(std::move(exception), init_no_addref()), false);
//
//                 f = hpx::traits::future_access<Future>::create(std::move(p));
            } else if (state == hpx::lcos::detail::future_state::invalid) {
//                 f = Future();
            } else {
                HPX_ASSERT(false);
            }
        }

        inline void serialize(hpx::serialization::output_archive& ar, boost::intrusive_ptr<treeture_task<void>> treeture_task_, int)
        {
            HPX_ASSERT(treeture_task_);
            ar << treeture_task_->parent_;

            int state = hpx::lcos::detail::future_state::invalid;
            if (ar.is_preprocessing())
            {
                if (!treeture_task_->is_ready())
                {
                    ar.await_future(treeture_task_);
                }
                else
                {
                    if (treeture_task_->has_value())
                    {
                        state = hpx::lcos::detail::future_state::has_value;
                        ar << state;
                    }
                    else if (treeture_task_->has_exception())
                    {
                        HPX_ASSERT(false);
//                         state = future_state::has_exception;
//                         boost::exception_ptr exception = treeture_task_->get_exception_ptr();
//                         ar << state << exception;
                    }
                    else
                    {
                        state = hpx::lcos::detail::future_state::invalid;
                        ar << state;
                    }
                }
                return;
            }

            if (treeture_task_->has_value())
            {
                state = hpx::lcos::detail::future_state::has_value;
                ar << state;
            }
            else if (treeture_task_->has_exception())
            {
                HPX_ASSERT(false);
//                 state = future_state::has_exception;
//                 boost::exception_ptr exception = treeture_task_->get_exception_ptr();
//                 ar << state << exception;
            }
            else
            {
                state = hpx::lcos::detail::future_state::invalid;
                ar << state;
            }
        }
    }
}

namespace hpx { namespace traits {
    template <typename T>
    struct future_access<boost::intrusive_ptr<allscale::detail::treeture_task<T>>>
    {
        typedef boost::intrusive_ptr<allscale::detail::treeture_task<T>> shared_state_type;
        static shared_state_type get_shared_state(shared_state_type ptr)
        {
            return ptr;
        }
    };

    template <typename T>
    struct get_remote_result<T, boost::intrusive_ptr<allscale::detail::treeture_task<T>>>
    {
        static T call(boost::intrusive_ptr<allscale::detail::treeture_task<T>> ptr)
        {
            HPX_ASSERT(ptr->has_value());
            return *ptr->get_result();
        }
    };

    template <>
    struct get_remote_result<void, boost::intrusive_ptr<allscale::detail::treeture_task<void>>>
    {
        static void call(boost::intrusive_ptr<allscale::detail::treeture_task<void>> ptr)
        {
            HPX_ASSERT(ptr->has_value());
            hpx::util::unused = *ptr->get_result();
        }
    };

    template <typename T>
    struct promise_local_result<boost::intrusive_ptr<allscale::detail::treeture_task<T>>>
    {
        typedef T type;
    };
}}

#endif
