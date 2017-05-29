/**
 * ------------------------ Auto-generated Code ------------------------ 
 *           This code was generated by the Insieme Compiler 
 * --------------------------------------------------------------------- 
 */
#include <allscale/runtime.hpp>
#include <array>
#include <iostream>
#include <memory>
#include <stdint.h>

#ifdef __cplusplus
#define INS_INIT(...) __VA_ARGS__
#else
#define INS_INIT(...) (__VA_ARGS__)
#endif
#ifdef __cplusplus
#include <new>
#define INS_INPLACE_INIT(Loc,Type) new(Loc) Type
#else
#define INS_INPLACE_INIT(Loc,Type) *(Loc) = (Type)
#endif
#ifdef __cplusplus
				/** Workaround for libstdc++/libc bug.
				 *  There's an inconsistency between libstdc++ and libc regarding whether
				 *  ::gets is declared or not, which is only evident when using certain
				 *  compilers and language settings
				 *  (tested positively with clang 3.9 --std=c++14 and libc 2.17).
				 */
				#include <initializer_list>  // force libstdc++ to include its config
				#undef _GLIBCXX_HAVE_GETS    // correct broken config
#endif

/* ------- Program Code --------- */

struct __wi_main_name {
    static const char* name() { return "__wi_main"; }
};

struct __wi_main_variant_1;
typedef struct __wi_main_variant_1 __wi_main_variant_1;

struct __wi_main_variant_0;
typedef struct __wi_main_variant_0 __wi_main_variant_0;

using __wi_main_work = allscale::work_item_description<int32_t, __wi_main_name, allscale::no_serialization, __wi_main_variant_0, __wi_main_variant_1 >;

/* ------- Function Definitions --------- */
int32_t main() {
    return allscale::runtime::main_wrapper<__wi_main_work >();
}

struct allscale_type_27;
typedef struct allscale_type_27 allscale_type_27;

struct allscale_type_27 {
    char data[3];;
};

struct allscale_type_28;
typedef struct allscale_type_28 allscale_type_28;

struct allscale_type_28 {
    char data[12];;
};

/* ------- Function Definitions --------- */
int32_t IMP_main() {
    const int32_t var_0 = 200;
    const int32_t var_1 = 100;
    const double var_2 = 1.0E-3;
    std::unique_ptr<struct std::array<struct std::array<double, 200 >, 200 >, struct std::default_delete<struct std::array<struct std::array<double, 200 >, 200 > > > var_3 = (std::unique_ptr<struct std::array<struct std::array<double, 200 >, 200 >, struct std::default_delete<struct std::array<struct std::array<double, 200 >, 200 > > >&&)std::make_unique<struct std::array<struct std::array<double, 200 >, 200 > >();
    std::unique_ptr<struct std::array<struct std::array<double, 200 >, 200 >, struct std::default_delete<struct std::array<struct std::array<double, 200 >, 200 > > > var_4 = (std::unique_ptr<struct std::array<struct std::array<double, 200 >, 200 >, struct std::default_delete<struct std::array<struct std::array<double, 200 >, 200 > > >&&)std::make_unique<struct std::array<struct std::array<double, 200 >, 200 > >();
    struct std::array<struct std::array<double, 200 >, 200 >& var_5 = var_3.operator*();
    {
        int32_t var_6 = 0;
        while (var_6 < var_0) {
            {
                int32_t var_7 = 0;
                while (var_7 < var_0) {
                    {
                        var_5.operator[]((uint64_t)var_6).operator[]((uint64_t)var_7) = (double)0;
                        if (var_6 == var_0 / 2 && var_7 == var_0 / 2) {
                            var_5.operator[]((uint64_t)var_6).operator[]((uint64_t)var_7) = (double)100;
                        };
                    };
                    var_7++;
                };
            };
            var_6++;
        };
    };
    {
        for (int32_t var_8 = 0; var_8 < var_1; ++var_8) {
            {
                struct std::array<struct std::array<double, 200 >, 200 >& var_9 = var_3.operator*();
                struct std::array<struct std::array<double, 200 >, 200 >& var_10 = var_4.operator*();
                {
                    for (int32_t var_11 = 1, _end = var_0 - 1; var_11 < _end; ++var_11) {
                        {
                            for (int32_t var_12 = 1, _end = var_0 - 1; var_12 < _end; ++var_12) {
                                var_10.operator[]((uint64_t)var_11).operator[]((uint64_t)var_12) = var_9.operator[]((uint64_t)var_11).operator[]((uint64_t)var_12) + var_2 * (var_9.operator[]((uint64_t)(var_11 - 1)).operator[]((uint64_t)var_12) + var_9.operator[]((uint64_t)(var_11 + 1)).operator[]((uint64_t)var_12) + var_9.operator[]((uint64_t)var_11).operator[]((uint64_t)(var_12 - 1)) + var_9.operator[]((uint64_t)var_11).operator[]((uint64_t)(var_12 + 1)) + (double)-4 * var_9.operator[]((uint64_t)var_11).operator[]((uint64_t)var_12));
                            };
                        };
                    };
                };
                if (var_8 % (var_1 / 10) == 0) {
                    std::operator<<(std::operator<<(std::cout, "t=").operator<<(var_8), " - center: ").operator<<(var_10.operator[]((uint64_t)(var_0 / 2)).operator[]((uint64_t)(var_0 / 2))).operator<<(&std::endl);
                };
                std::swap(var_3, var_4);
            };
        };
    };
    std::operator<<(std::operator<<(std::cout, "t=").operator<<(var_1), " - center: ").operator<<(var_3.operator*().operator[]((uint64_t)(var_0 / 2)).operator[]((uint64_t)(var_0 / 2))).operator<<(&std::endl);
    return var_3.operator*().operator[]((uint64_t)(var_0 / 2)).operator[]((uint64_t)(var_0 / 2)) < (double)69?0:1;
}
ALLSCALE_REGISTER_TREETURE_TYPE(int32_t)
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_1(hpx::util::tuple< > const& var_0) {
    return allscale::treeture<int32_t >(IMP_main());
}
struct __wi_main_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple< > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_main_variant_1::execute(hpx::util::tuple< > const& var_0) {
    return allscale_fun_1(var_0);
}
struct __wi_main_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple< > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_main_variant_0::execute(hpx::util::tuple< > const& var_0) {
    return allscale_fun_1(var_0);
}
