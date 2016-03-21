
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
    struct treeture
      : hpx::components::client_base<treeture<T>, components::treeture<T>>
    {
        using base_type = hpx::components::client_base<treeture<T>, components::treeture<T>>;
        using result_type = T;
        using future_type = hpx::future<T>;

        using base_type::gid_;

        using set_value_action =
            typename components::treeture<T>::set_value_action;
        using get_future_action =
            typename components::treeture<T>::get_future_action;

        treeture()
          : base_type(hpx::new_<components::treeture<T>>(hpx::find_here()))
        {}

        template <
            typename F,
            typename Enable =
                typename std::enable_if<
                    hpx::traits::is_future<F>::value
                 && !std::is_same<typename hpx::util::decay<F>::type, treeture>::value
                >::type
        >
        explicit treeture(F f)
          : base_type(hpx::new_<components::treeture<T>>(hpx::find_here()))
        {
            this->get_gid();
            hpx::when_all(gid_, std::move(f)).then(
                [](hpx::shared_future<hpx::id_type> const & gid, F f)
                {
                    hpx::apply<set_value_action>(
                        gid.get(), f.get()
                    );
                }
            );
        }

        template <
            typename U,
            typename Enable =
                typename std::enable_if<
                    !hpx::traits::is_future<U>::value
                 && !std::is_same<typename hpx::util::decay<U>::type, treeture>::value
                >::type
        >
        explicit treeture(U && u)
          : base_type(hpx::new_<components::treeture<T>>(hpx::find_here(), std::forward<U>(u)))
        {
        }

        treeture(treeture && o)
          : base_type(std::move(o.gid_))
        {}

        treeture(treeture const& o)
          : base_type(o.gid_)
        {}

        explicit treeture(hpx::future<treeture> && f)
          : base_type(std::move(f))
        {
            HPX_ASSERT(gid_.valid());
        }

        void set_value(T && t)
        {
            this->get_gid();
            gid_.then(
                hpx::util::bind(
                    hpx::util::one_shot([](hpx::shared_future<hpx::id_type> const & gid, T && u)
                    {
                        hpx::apply<set_value_action>(
                            gid.get(), std::move(u)
                        );
                    }),
                    hpx::util::placeholders::_1,
                    std::move(t)
                )
            );
//             this->reset();
        }

        hpx::future<T> get_future()
        {
            this->get_gid();
//             hpx::shared_future<hpx::id_type> g = gid_;
//             this->reset();
            return gid_.then(
                [](hpx::shared_future<hpx::id_type> gid)
                {
                    return hpx::async<get_future_action>(gid.get());
                }
            );
        }

        T get()
        {
            return get_future().get();
        }
    };

    namespace traits
    {
        template <typename T>
        struct is_treeture< ::allscale::treeture<T>>
          : std::true_type
        {};
    }
}

#define ALLSCALE_REGISTER_TREETURE_TYPE_(T, NAME)                               \
    using NAME =                                                                \
        ::hpx::components::component<                                           \
            ::allscale::components::treeture< T >                               \
        >;                                                                      \
    HPX_REGISTER_COMPONENT(NAME)                                                \
/**/

#define ALLSCALE_REGISTER_TREETURE_TYPE(T)                                      \
    ALLSCALE_REGISTER_TREETURE_TYPE_(                                           \
        T,                                                                      \
        BOOST_PP_CAT(BOOST_PP_CAT(treeture, T), _component_type)                \
    )                                                                           \
/**/

#endif
