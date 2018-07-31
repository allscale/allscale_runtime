
#ifndef ALLSCALE_TREETURE_HPP
#define ALLSCALE_TREETURE_HPP

#include <allscale/treeture_fwd.hpp>
#include <hpx/runtime/components/server/component.hpp>
#include <hpx/runtime/components/new.hpp>
#include <hpx/lcos/base_lco_with_value.hpp>
#include <hpx/lcos/async.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/async.hpp>
#include <hpx/apply.hpp>

namespace allscale {
    template <typename T>
    struct treeture
    {
    private:
        typedef detail::treeture_lco<T> wrapped_type;

        typedef detail::treeture_task<T> shared_state_type;

        typedef hpx::lcos::local::spinlock mutex_type;

    public:
        using future_type = hpx::future<T>;

        treeture()
          : shared_state_(nullptr)
        {}

        treeture(treeture_init_t)
          : shared_state_(new shared_state_type(
                typename shared_state_type::init_no_addref()))
        {
        }

        template <typename U, typename Enable =
            typename std::enable_if<
                !hpx::traits::is_future<U>::value
             && !traits::is_treeture<U>::value
            >::type>
        explicit treeture(U&& u)
          : shared_state_(new shared_state_type(std::forward<U>(u),
                typename shared_state_type::init_no_addref()))
        {
        }

        template <typename U>
        treeture(treeture<U> other,
            typename std::enable_if<std::is_void<T>::value, U>::type* = nullptr)
          : shared_state_(new shared_state_type(
                typename shared_state_type::init_no_addref()))
        {
            hpx::future<void> f = other.get_future();

            typename hpx::traits::detail::shared_state_ptr_for<hpx::future<void>>::type state
                = hpx::traits::future_access<hpx::future<void>>::get_shared_state(f);

//             set_left_child(other.get_left_child());
//             set_right_child(other.get_right_child());
            auto shared_state = shared_state_;
            state->set_on_completed(
                [state, shared_state]()
                {
                    shared_state->set_value(hpx::util::unused_type{});
                });
        }

        template <typename U>
        treeture(hpx::future<U> fut, typename std::enable_if<!std::is_same<T, void>::value, U>::type* = nullptr)
          : shared_state_(new shared_state_type(
                typename shared_state_type::init_no_addref()))
        {
            HPX_ASSERT(fut.valid());
            typename hpx::traits::detail::shared_state_ptr_for<hpx::future<U>>::type state
                = hpx::traits::future_access<hpx::future<U>>::get_shared_state(fut);

            auto shared_state = shared_state_;
            state->set_on_completed(
                [state, shared_state]()
                {
                    shared_state->set_value(std::move(*state->get_result()));
                });
        }

        template <typename U>
        treeture(hpx::future<U> fut, typename std::enable_if<std::is_same<T, void>::value, U>::type* = nullptr)
          : shared_state_(new shared_state_type(
                typename shared_state_type::init_no_addref()))
        {
            typename hpx::traits::detail::shared_state_ptr_for<hpx::future<U>>::type state
                = hpx::traits::future_access<hpx::future<U>>::get_shared_state(fut);

            auto shared_state = shared_state_;
            state->set_on_completed(
                [state, shared_state]()
                {
                    shared_state->set_value(hpx::util::unused);
                });
        }

        treeture(treeture<T> const& t)
          : shared_state_(t.shared_state_),
            id_(t.id_)
          , fixed_children_(t.fixed_children_)
        {
        }

        treeture(treeture<T>&& t) noexcept
          : shared_state_(std::move(t.shared_state_)),
            id_(std::move(t.id_))
          , fixed_children_(t.fixed_children_)
        {
        }

        treeture& operator=(treeture<T> const& t)
        {
            shared_state_ = t.shared_state_;
            id_ = t.id_;
            fixed_children_ = t.fixed_children_;

            return *this;
        }

        treeture& operator=(treeture<T>&& t) noexcept
        {
            shared_state_ = std::move(t.shared_state_);
            id_ = std::move(t.id_);
            fixed_children_ = t.fixed_children_;

            return *this;
        }

        hpx::naming::id_type get_id()
        {
            return id_;
        }

        bool valid() const
        {
            return shared_state_ != nullptr || (id_ && id_ != hpx::invalid_id);
        }

        explicit operator bool() const
        {
            return valid();
        }

        explicit operator hpx::future<T>() const
        {
            return get_future();
        }

        void wait() const
        {
            if (shared_state_)
            {
                shared_state_->wait();
            }
            else
            {
                typename wrapped_type::get_state_action action;
                HPX_ASSERT(id_);
                action(id_);
            }
        }

        template <typename U>
        void set_value(U&& u)
        {
            if (shared_state_)
            {
                // Try setting the state, we'll catch any exception in the process
                // and ignore them for now...
                try {
                    shared_state_->set_value(std::forward<U>(u));
                }
                catch (...)
                {
                }
            }
            else
            {
                typename detail::treeture_lco<T>::set_value_action action;
                HPX_ASSERT(id_);
                hpx::apply(action, id_, std::forward<U>(u));
            }
        }

        hpx::future<T> get_future() const
        {
            if (shared_state_)
            {
                return hpx::traits::future_access<hpx::future<T>>
                    ::create(shared_state_);
            }
            else
            {
                typename wrapped_type::get_state_action action;
                HPX_ASSERT(id_);
                return hpx::async(action, id_);
            }
        }

        T get_result() const
        {
            return get_future().get();
        }

        void set_children(treeture<void> left, treeture<void> right)
        {
//             if (shared_state_)
//             {
//                 shared_state_->set_children(std::move(left), std::move(right));
//             }
//             else
//             {
//                 typename wrapped_type::set_children_action action;
//                 hpx::apply(action, id_, std::move(left), std::move(right));
//             }
        }

        void set_child(std::size_t idx, treeture<void> child)
        {
            // FIXME in future versions ... the compiler only generates
            // binary trees
            if (idx > 1) return;

            if (shared_state_)
            {
                switch (idx)
                {
                    case 0:
                        shared_state_->set_left_child(std::move(child));
                        break;
                    case 1:
                        shared_state_->set_right_child(std::move(child));
                        break;
                    default:
                        HPX_ASSERT(false);
                        break;
                }
                return;
            }
            switch (idx)
            {
                case 0:
                    {
                        typename wrapped_type::set_left_child_action action;
                        hpx::apply(action, id_, std::move(child));
                    }
                    break;
                case 1:
                    {
                        typename wrapped_type::set_right_child_action action;
                        hpx::apply(action, id_, std::move(child));
                    }
                    break;
                default:
                    HPX_ASSERT(false);
                    break;
            }

        }

        treeture<void> get_left_child() const
        {
            if (fixed_children_) return *this;

            if (shared_state_)
            {
                auto const& res = shared_state_->get_left_child();
                if(!res.valid())
                {
                    treeture<void> t(*this);
                    t.fixed_children_ = true;
                    return t;
                }
                return res;
            }
            else
            {
                typename wrapped_type::get_left_child_action action;
                // TODO: futurize
                return hpx::async(action, id_);
            }
        }

        treeture<void> get_right_child() const
        {
            if(fixed_children_) return *this;

            if (shared_state_)
            {
                auto res = shared_state_->get_right_child();
                if(!res.valid())
                {
                    treeture<void> t(*this);
                    t.fixed_children_ = true;
                    return t;
                }
                return res;
            }
            {
                typename wrapped_type::get_right_child_action action;
                // TODO: futurize
                return hpx::async(action, id_);
            }
        }

        template <typename Archive>
        void load(Archive & ar, unsigned)
        {
            HPX_ASSERT(!shared_state_);
            ar & id_;
            ar & fixed_children_;
        }

        template <typename Archive>
        void save(Archive & ar, unsigned) const
        {
            if (shared_state_)
            {
                std::unique_lock<mutex_type> l(mtx_);
                hpx::id_type id;
                if (!id_)
                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    id = hpx::local_new<wrapped_type>(shared_state_).get();
                    id.make_unmanaged();
                }
                if (!id_)
                {
                    id_ = std::move(id);
                }
            }
            ar & id_;
            ar & fixed_children_;
        }

         HPX_SERIALIZATION_SPLIT_MEMBER();

    private:
        boost::intrusive_ptr<shared_state_type> shared_state_;
        mutable mutex_type mtx_;
        mutable hpx::id_type id_;
        bool fixed_children_ = false;

        template <typename U>
        friend struct treeture;

        template <typename U>
        friend struct hpx::traits::future_access;
    };

    template <typename T>
    inline treeture<T> make_ready_treeture(T&& t)
    {
        return treeture<T>(std::forward<T>(t));
    }

    inline treeture<void> make_ready_treeture()
    {
        return treeture<void>(hpx::util::unused);
    }
}

namespace hpx { namespace traits {
    namespace detail {
        template <typename T>
        struct shared_state_ptr_for<allscale::treeture<T>>
        {
            typedef boost::intrusive_ptr<allscale::detail::treeture_task<T>> type;
        };
    }

//     template <typename T>
//     struct is_future<allscale::treeture<T>> : std::true_type {};

    template <typename T>
    struct future_access<allscale::treeture<T>>
    {
        typedef boost::intrusive_ptr<allscale::detail::treeture_task<T>> shared_state_type;
        static shared_state_type get_shared_state(allscale::treeture<T> t)
        {
            HPX_ASSERT(t.shared_state_);
            return t.shared_state_;
        }
    };
}}

#include <allscale/detail/treeture_lco.hpp>
#include <allscale/detail/treeture_task.hpp>
#include <hpx/runtime/components/component_factory.hpp>

#define ALLSCALE_REGISTER_TREETURE_TYPE_(T, NAME)                               \
    using HPX_PP_CAT(NAME, _treeture) = ::allscale::detail::treeture_lco< T >;  \
    using HPX_PP_CAT(NAME, _treeture_component) =                               \
        ::hpx::components::component<                                           \
            HPX_PP_CAT(NAME, _treeture)                                         \
        >;                                                                      \
    HPX_REGISTER_COMPONENT(HPX_PP_CAT(NAME, _treeture_component),               \
        HPX_PP_CAT(NAME, _treeture))                                            \
/**/

#define ALLSCALE_REGISTER_TREETURE_TYPE(T)                                      \
    ALLSCALE_REGISTER_TREETURE_TYPE_(                                           \
        T, T                                                                    \
    )                                                                           \
/**/

#endif
