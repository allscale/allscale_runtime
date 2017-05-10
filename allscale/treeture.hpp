
#ifndef ALLSCALE_TREETURE_HPP
#define ALLSCALE_TREETURE_HPP

#include <allscale/components/treeture.hpp>
#include <allscale/traits/is_treeture.hpp>

#include <hpx/include/future.hpp>
#include <hpx/include/lcos.hpp>

#include <utility>

namespace allscale
{
    template <typename T>
    struct treeture;
}

namespace hpx { namespace traits {
    template <typename R>
    struct is_future< ::allscale::treeture<R>>
      : std::false_type
    {};
}}

namespace allscale
{
    struct treeture_base
    {
        virtual ~treeture_base() {}
    };
    template <typename T>
    struct treeture
      : hpx::components::client_base<treeture<T>, components::treeture<T>>,
        treeture_base
    {
        using base_type = hpx::components::client_base<treeture<T>, components::treeture<T>>;
        using result_type = T;
        using future_type = hpx::future<T>;

        using set_value_action =
            typename components::treeture<T>::set_value_action;
        using attach_promise_action =
            typename components::treeture<T>::attach_promise_action;
        using get_left_neighbor_action =
            typename components::treeture_base::get_left_neighbor_action;
        using get_right_neighbor_action =
            typename components::treeture_base::get_right_neighbor_action;
        using set_child_action =
            typename components::treeture_base::set_child_action;

        treeture()
        {}

        treeture(hpx::id_type loc, hpx::id_type parent = hpx::invalid_id)
          : base_type(hpx::new_<components::treeture<T>>(loc, parent))
        {
            HPX_ASSERT(this->valid());
        }

        template <
            typename F,
            typename Enable =
                typename std::enable_if<
                    hpx::traits::is_future<F>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, treeture>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, hpx::future<treeture>>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, hpx::future<hpx::id_type>>::value
                >::type
        >
        explicit treeture(F f, hpx::id_type parent = hpx::invalid_id)
          : base_type(hpx::new_<components::treeture<T>>(hpx::find_here(), parent))
        {
            HPX_ASSERT(this->valid());
            hpx::id_type this_id = this->get_id();
            f.then(
                [this_id](F f)
                {
                    hpx::apply<set_value_action>(
                        this_id, f.get()
                    );
                }
            );
            HPX_ASSERT(this->valid());
        }

        template <
            typename U,
            typename Enable =
                typename std::enable_if<
                    !hpx::traits::is_future<U>::value
                 && !std::is_same<typename hpx::util::decay<U>::type, treeture>::value
                 && !std::is_same<typename hpx::util::decay<U>::type, hpx::future<treeture>>::value
                 && !std::is_same<typename hpx::util::decay<U>::type, hpx::future<hpx::id_type>>::value
                >::type
        >
        explicit treeture(U && u, hpx::id_type parent = hpx::invalid_id)
          : base_type(hpx::new_<components::treeture<T>>(hpx::find_here(), std::forward<U>(u), parent))
        {
            HPX_ASSERT(this->valid());
        }

        treeture(treeture && o)
          : base_type(std::move(o))
        {
            HPX_ASSERT(this->valid());
        }

        treeture(treeture const& o)
          : base_type(o)
        {
            HPX_ASSERT(this->valid());
        }

        explicit treeture(hpx::future<treeture> && f)
          : base_type(std::move(f))
        {
            HPX_ASSERT(this->valid());
//             HPX_ASSERT(this->get_id().valid());
        }

        explicit treeture(hpx::future<hpx::id_type> && f)
          : base_type(std::move(f))
        {
            HPX_ASSERT(this->valid());
        }

        treeture& operator=(treeture&& other) {
            base_type::operator=(std::move(other));
            return *this;
        }

        void set_value(T && t)
        {
            HPX_ASSERT(this->valid());
            hpx::apply<set_value_action>(
                this->get_id(), std::move(t)
            );
//             this->reset();
        }

        hpx::future<T> get_future() const
        {
            HPX_ASSERT(this->valid());
            hpx::lcos::promise<T> p;
            hpx::future<T> fut = p.get_future();
            hpx::apply<attach_promise_action>(
                this->get_id(), p.get_id(), p.resolve());
            return std::move(fut);
        }

        explicit operator hpx::future<T>()
        {
            return get_future();
        }

        T get_result()
        {
            HPX_ASSERT(this->valid());
            return get_future().get();
        }

        void wait() const
        {
            // TODO: this fails in the fine_grained cases since the future has been retrieved before
            // How else can I wait for the completion of the task?

            get_future().wait();     // TODO: provide replacement
        }

        void set_child(std::size_t idx, hpx::id_type child)
        {
            hpx::apply<set_child_action>(this->get_id(), idx, child);
        }

        treeture get_left_child() const {
            return treeture(
                hpx::async<get_left_neighbor_action>(this->get_id()));
        }

        treeture get_right_child() const {
            return treeture(
                hpx::async<get_right_neighbor_action>(this->get_id()));
        }

         template <typename Archive>
         void load(Archive & ar, unsigned)
         {
             ar & hpx::serialization::base_object<base_type>(*this);
             HPX_ASSERT(this->valid());
         }

         template <typename Archive>
         void save(Archive & ar, unsigned) const
         {
             HPX_ASSERT(this->valid());
             ar & hpx::serialization::base_object<base_type>(*this);
             HPX_ASSERT(this->valid());
         }

         HPX_SERIALIZATION_SPLIT_MEMBER();
    };

    template <typename T>
    inline treeture<typename std::decay<T>::type> make_ready_treeture(T&& t)
    {
        return treeture<typename std::decay<T>::type>(std::forward<T>(t));
    }

    template <>
    struct treeture<void>
      : public treeture<hpx::util::unused_type>
    {
        using base_type = treeture<hpx::util::unused_type>;
        using result_type = void;
        using future_type = hpx::future<void>;

        using set_value_action =
            typename components::treeture<hpx::util::unused_type>::set_value_action;
        using attach_promise_action =
            typename components::treeture<hpx::util::unused_type>::attach_promise_action;

        treeture()
        {}

        treeture(hpx::id_type loc, hpx::id_type parent = hpx::invalid_id)
          : base_type(loc, parent)
        {
            HPX_ASSERT(this->valid());
        }

        template <
            typename F,
            typename Enable =
                typename std::enable_if<
                    hpx::traits::is_future<F>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, treeture>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, hpx::future<treeture>>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, hpx::future<hpx::id_type>>::value
                >::type
        >
        explicit treeture(F f, hpx::id_type parent = hpx::invalid_id)
          : base_type(hpx::find_here(), parent)
        {
            HPX_ASSERT(this->valid());
            hpx::id_type this_id = this->get_id();
            f.then(
                [this_id](F f)
                {
                    hpx::apply<set_value_action>(
                        this_id, hpx::util::unused_type()
                    );
                }
            );
            HPX_ASSERT(this->valid());
        }

        explicit treeture(hpx::util::unused_type, hpx::id_type parent = hpx::invalid_id)
          : base_type(hpx::util::unused_type(), parent)
        {
            HPX_ASSERT(this->valid());
        }

        treeture(treeture && o)
          : base_type(std::move(static_cast<base_type &&>(o)))
        {
            HPX_ASSERT(this->valid());
        }

        treeture(treeture const& o)
          : base_type(static_cast<base_type const&>(o))
        {
            HPX_ASSERT(this->valid());
        }

        explicit treeture(hpx::future<treeture> && f)
          : base_type(std::move(f))
        {
            HPX_ASSERT(this->valid());
//             HPX_ASSERT(this->get_id().valid());
        }

        explicit treeture(hpx::future<hpx::id_type> && f)
          : base_type(std::move(f))
        {
            HPX_ASSERT(this->valid());
        }

        treeture& operator=(treeture&& other) {
            base_type::operator=(std::move(other));
            return *this;
        }

        void set_value()
        {
            HPX_ASSERT(this->valid());
            hpx::apply<set_value_action>(
                this->get_id(), hpx::util::unused_type()
            );
//             this->reset();
        }

        hpx::future<void> get_future() const
        {
            HPX_ASSERT(this->valid());
            hpx::lcos::promise<void> p;
            hpx::future<void> fut = p.get_future();
            hpx::apply<attach_promise_action>(
                this->get_id(), p.get_id(), p.resolve());
            return std::move(fut);
        }

        explicit operator hpx::future<void>()
        {
            return get_future();
        }

        void get_result()
        {
            HPX_ASSERT(this->valid());
            get_future().get();
        }

        void wait() const
        {
            // TODO: this fails in the fine_grained cases since the future has been retrieved before
            // How else can I wait for the completion of the task?

            get_future().wait();     // TODO: provide replacement
        }

        void set_child(std::size_t idx, hpx::id_type child)
        {
            hpx::apply<set_child_action>(this->get_id(), idx, child);
        }

        treeture get_left_child() const {
            return treeture(
                hpx::async<get_left_neighbor_action>(this->get_id()));
        }

        treeture get_right_child() const {
            return treeture(
                hpx::async<get_right_neighbor_action>(this->get_id()));
        }
    };

    inline treeture<void> make_ready_treeture()
    {
        return treeture<void>(hpx::util::unused_type());
    }

    template <typename R>
    using task_reference = treeture<R>;

    namespace traits
    {
        template <typename T>
        struct is_treeture< ::allscale::treeture<T>>
          : std::true_type
        {};
    }
}

#define ALLSCALE_REGISTER_TREETURE_TYPE_(T, NAME)                               \
    using BOOST_PP_CAT(NAME, _treeture) = ::allscale::components::treeture< T >;\
    using NAME =                                                                \
        ::hpx::components::managed_component<                                   \
            BOOST_PP_CAT(NAME, _treeture)                                       \
        >;                                                                      \
    HPX_REGISTER_DERIVED_COMPONENT_FACTORY(NAME, BOOST_PP_CAT(NAME, _treeture), "treeture_base")\
/**/

#define ALLSCALE_REGISTER_TREETURE_TYPE(T)                                      \
    ALLSCALE_REGISTER_TREETURE_TYPE_(                                           \
        T,                                                                      \
        BOOST_PP_CAT(BOOST_PP_CAT(treeture, T), _component_type)                \
    )                                                                           \
/**/

#endif
