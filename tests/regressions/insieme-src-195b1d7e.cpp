/**
 * ------------------------ Auto-generated Code ------------------------
 *           This code was generated by the Insieme Compiler
 * ---------------------------------------------------------------------
 */
#include <allscale/runtime.hpp>
#include <stdbool.h>
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

struct __wi_main_variant_0;
typedef struct __wi_main_variant_0 __wi_main_variant_0;

struct __wi_main_variant_1;
typedef struct __wi_main_variant_1 __wi_main_variant_1;

using __wi_main_work = allscale::work_item_description<int32_t, __wi_main_name, allscale::no_serialization, __wi_main_variant_0, __wi_main_variant_1 >;

/* ------- Function Definitions --------- */
int32_t main() {
    return allscale::runtime::main_wrapper<__wi_main_work >();
}

ALLSCALE_REGISTER_TREETURE_TYPE(int32_t)
struct __wi_allscale_wi_0_name {
    static const char* name() { return "__wi_allscale_wi_0"; }
};

struct __wi_allscale_wi_0_variant_1;
typedef struct __wi_allscale_wi_0_variant_1 __wi_allscale_wi_0_variant_1;

struct __wi_allscale_wi_0_variant_0;
typedef struct __wi_allscale_wi_0_variant_0 __wi_allscale_wi_0_variant_0;

using __wi_allscale_wi_0_work = allscale::work_item_description<int32_t, __wi_allscale_wi_0_name, allscale::no_serialization, __wi_allscale_wi_0_variant_0, __wi_allscale_wi_0_variant_1 >;

/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_5(allscale::runtime::dependencies const& var_0, hpx::util::tuple<int32_t > const& var_1) {
    return allscale::spawn<__wi_allscale_wi_0_work >(hpx::util::get<0 >(var_1));
}
/* ------- Function Definitions --------- */
int32_t allscale_fun_4() {
    allscale::runtime::make_prec_operation<int32_t, int32_t >(INS_INIT(hpx::util::tuple< >){}, &allscale_fun_5);
    return 0;
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_2(hpx::util::tuple<int, char** > const& var_0) {
    return allscale::treeture<int32_t >(allscale_fun_4());
}
struct __wi_main_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int, char** > const& var_0);
    static constexpr bool valid = true;
};

/* ------- Function Prototypes ---------- */
int32_t rec(hpx::util::tuple<int32_t > const& p1);

/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_14(hpx::util::tuple<int32_t > const& var_0) {
    return allscale::treeture<int32_t >(rec(var_0));
}
struct __wi_allscale_wi_0_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int32_t > const& var_0);
    static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
bool allscale_fun_9(hpx::util::tuple<int32_t > const& var_0) {
    return hpx::util::get<0 >(var_0) < 2;
}
/* ------- Function Definitions --------- */
int32_t allscale_fun_11(hpx::util::tuple<int32_t > const& var_0) {
    return hpx::util::get<0 >(var_0);
}
/* ------- Function Definitions --------- */
int32_t allscale_fun_15(hpx::util::tuple<int32_t > const& var_0) {
    int32_t a = rec(INS_INIT(hpx::util::tuple<int32_t >){hpx::util::get<0 >(var_0) - 1});
    int32_t b = rec(INS_INIT(hpx::util::tuple<int32_t >){hpx::util::get<0 >(var_0) - 2});
    return a + b;
}
/* ------- Function Definitions --------- */
int32_t rec(hpx::util::tuple<int32_t > const& var_0) {
    if (allscale_fun_9(var_0)) {
        return allscale_fun_11(var_0);
    } else {
        return allscale_fun_15(var_0);
    };
}
allscale::treeture<int32_t > __wi_allscale_wi_0_variant_1::execute(hpx::util::tuple<int32_t > const& var_0) {
    return allscale_fun_14(var_0);
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_13(hpx::util::tuple<int32_t > const& var_0) {
    allscale::treeture<int32_t > a = (allscale::spawn<__wi_allscale_wi_0_work >)(hpx::util::get<0 >(var_0) - 1);
    allscale::treeture<int32_t > b = (allscale::spawn<__wi_allscale_wi_0_work >)(hpx::util::get<0 >(var_0) - 2);
    return allscale::treeture<int32_t >(a.get_result() + b.get_result());
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_7(hpx::util::tuple<int32_t > const& var_0) {
    if (allscale_fun_9(var_0)) {
        return allscale::treeture<int32_t >(allscale_fun_11(var_0));
    } else {
        return allscale_fun_13(var_0);
    };
}
struct __wi_allscale_wi_0_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int32_t > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_allscale_wi_0_variant_0::execute(hpx::util::tuple<int32_t > const& var_0) {
    return allscale_fun_7(var_0);
}
allscale::treeture<int32_t > __wi_main_variant_0::execute(hpx::util::tuple<int, char** > const& var_0) {
    return allscale_fun_2(var_0);
}
struct __wi_main_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int, char** > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_main_variant_1::execute(hpx::util::tuple<int, char** > const& var_0) {
    return allscale_fun_2(var_0);
}


