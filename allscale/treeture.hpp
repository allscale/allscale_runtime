
#ifndef ALLSCALE_TREETURE_HPP
#define ALLSCALE_TREETURE_HPP

#include <allscale/treeture_fwd.hpp>

#include <hpx/lcos/base_lco_with_value.hpp>
#include <hpx/runtime/components/new.hpp>
#include <hpx/include/components.hpp>
#include <hpx/lcos/async.hpp>
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
          : fixed_children_(false)
        {}

        treeture(parent_arg, treeture<void> const& parent = treeture<void>())
          : shared_state_(new shared_state_type(parent,
                typename shared_state_type::init_no_addref()))
          , fixed_children_(false)
        {
        }

        template <typename U, typename Enable =
            typename std::enable_if<
                !hpx::traits::is_future<U>::value
             && !traits::is_treeture<U>::value
            >::type>
        explicit treeture(U&& u, treeture<void> const& parent = treeture<void>())
          : shared_state_(new shared_state_type(parent, std::forward<U>(u),
                typename shared_state_type::init_no_addref()))
          , fixed_children_(false)
        {
        }

        template <typename U>
        treeture(treeture<U> other,
            typename std::enable_if<std::is_void<T>::value, U>::type* = nullptr)
          : shared_state_(new shared_state_type(other.parent(),
                typename shared_state_type::init_no_addref()))
          , fixed_children_(false)
        {
//             set_left_child(other.get_left_child());
//             set_right_child(other.get_right_child());
            auto shared_state = shared_state_;
            if (other.shared_state_)
            {
                other.shared_state_->set_on_completed(
                    [shared_state]()
                    {
                        shared_state->set_value(hpx::util::unused_type{});
                    });
            }
            else
            {
                hpx::future<void> f = other.get_future();

                typename hpx::traits::detail::shared_state_ptr_for<hpx::future<void>>::type state
                    = hpx::traits::future_access<hpx::future<void>>::get_shared_state(f);
                state->set_on_completed(
                    [state, shared_state]()
                    {
                        shared_state->set_value(hpx::util::unused);
                    });
            }
        }

        template <typename U>
        treeture(hpx::future<U> fut, typename std::enable_if<!std::is_same<T, void>::value, U>::type* = nullptr)
          : shared_state_(new shared_state_type(treeture<void>(),
                typename shared_state_type::init_no_addref()))
          , fixed_children_(false)
        {
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
          : shared_state_(new shared_state_type(treeture<void>(),
                typename shared_state_type::init_no_addref()))
          , fixed_children_(false)
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

        treeture(treeture<T>&& t)
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

        treeture& operator=(treeture<T>&& t)
        {
            shared_state_ = std::move(t.shared_state_);
            id_ = std::move(t.id_);
            fixed_children_ = t.fixed_children_;

            return *this;
        }

        hpx::naming::id_type get_id()
        {
            return id_.get();
        }

        bool valid() const
        {
            return shared_state_ != nullptr || (id_.valid() && id_.get() != hpx::invalid_id);
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
                HPX_ASSERT(id_.valid());
                action(id_.get());
            }
        }

        template <typename U>
        void set_value(U&& u)
        {
            if (shared_state_)
            {
                shared_state_->set_value(std::forward<U>(u));
            }
            else
            {
                typename detail::treeture_lco<T>::set_value_action action;
                HPX_ASSERT(id_.valid());
                hpx::apply(action, id_.get(), std::forward<U>(u));
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
                HPX_ASSERT(id_.valid());
                return hpx::async(action, id_.get());
            }
        }

        T get_result() const
        {
            return get_future().get();
        }

        treeture<void> parent()
        {
            if (shared_state_)
            {
//                 HPX_ASSERT(shared_state_->parent_);
                return shared_state_->parent_;
            }
            typename wrapped_type::get_parent_action action;
//             return hpx::async(action, id_);
            return action(id_.get());
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
            hpx::id_type const& dest = id_.get();
            switch (idx)
            {
                case 0:
                    {
                        typename wrapped_type::set_left_child_action action;
                        hpx::apply(action, dest, std::move(child));
                    }
                    break;
                case 1:
                    {
                        typename wrapped_type::set_right_child_action action;
                        hpx::apply(action, dest, std::move(child));
                    }
                    break;
                default:
                    HPX_ASSERT(false);
                    break;
            }

        }

        treeture<void> get_left_child() const
        {
            if(fixed_children_) return *this;

            if (shared_state_)
            {
                auto res = shared_state_->get_left_child();
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
                return hpx::async(action, id_.get());
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
                return hpx::async(action, id_.get());
            }
        }

        template <typename Archive>
        void load(Archive & ar, unsigned)
        {
            HPX_ASSERT(!shared_state_);
            bool valid = true;
            ar & valid;
            if (valid)
            {
                ar & id_;
            }
            ar & fixed_children_;
        }

        template <typename Archive>
        void save(Archive & ar, unsigned) const
        {
            bool valid = id_.valid();
            {
                std::unique_lock<mutex_type> l(mtx_);
                if (shared_state_ && !id_.valid())
                {
                    hpx::shared_future<hpx::id_type> id;
                    {
                        hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                        id = hpx::local_new<wrapped_type>(shared_state_);
                    }
                    if (!id_.valid()) id_ = id;
                    valid = true;
                }
            }
            ar & valid;
            if (valid)
            {
                ar & id_;
            }
            ar & fixed_children_;
        }

         HPX_SERIALIZATION_SPLIT_MEMBER();

    private:
        boost::intrusive_ptr<shared_state_type> shared_state_;
        mutable mutex_type mtx_;
        mutable hpx::shared_future<hpx::id_type> id_;
        bool fixed_children_;

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

#define ALLSCALE_REGISTER_TREETURE_TYPE_(T, NAME)                               \
    using BOOST_PP_CAT(NAME, _treeture) = ::allscale::detail::treeture_lco< T >;\
    using NAME =                                                                \
        ::hpx::components::managed_component<                                   \
            BOOST_PP_CAT(NAME, _treeture)                                       \
        >;                                                                      \
    HPX_REGISTER_DERIVED_COMPONENT_FACTORY(NAME, BOOST_PP_CAT(NAME, _treeture), \
        BOOST_PP_STRINGIZE(BOOST_PP_CAT(NAME, treeture_base_lco)))              \
/**/

#define ALLSCALE_REGISTER_TREETURE_TYPE(T)                                      \
    ALLSCALE_REGISTER_TREETURE_TYPE_(                                           \
        T,                                                                      \
        BOOST_PP_CAT(BOOST_PP_CAT(treeture, T), _component_type)                \
    )                                                                           \
/**/

#endif
