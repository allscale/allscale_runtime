#include <algorithm>
#include <alloca.h>
#include <allscale/api/user/data/binary_tree.h>
#include <allscale/runtime.hpp>
#include <allscale/utils/serializer.h>
#include <allscale/utils/vector.h>
#include <array>
#include <assert.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdbool.h>
#include <stdint.h>
#include <utility>
#include <vector>

#ifdef __cplusplus
#define INS_INIT(...) __VA_ARGS__
#else
#define INS_INIT(...) (__VA_ARGS__)
#endif
#ifdef __cplusplus
#include <new>
#define INS_INPLACE_INIT(Loc, Type) new (Loc) Type
#else
#define INS_INPLACE_INIT(Loc, Type) *(Loc) = (Type)
#endif
#ifdef __cplusplus
/** Workaround for libstdc++/libc bug.
 *  There's an inconsistency between libstdc++ and libc regarding whether
 *  ::gets is declared or not, which is only evident when using certain
 *  compilers and language settings
 *  (tested positively with clang 3.9 --std=c++14 and libc 2.17).
 */
#include <initializer_list> // force libstdc++ to include its config
#undef _GLIBCXX_HAVE_GETS   // correct broken config
#endif

/* ------- Program Code --------- */

struct __wi_main_variant_0;
typedef struct __wi_main_variant_0 __wi_main_variant_0;

struct __wi_main_name {
  static const char *name() { return "__wi_main"; }
};

struct __wi_main_can_split;
typedef struct __wi_main_can_split __wi_main_can_split;

struct __wi_main_variant_1;
typedef struct __wi_main_variant_1 __wi_main_variant_1;

using __wi_main_work = allscale::work_item_description<
    int32_t, __wi_main_name, allscale::no_serialization, __wi_main_variant_0,
    __wi_main_variant_1, __wi_main_can_split>;

/* ------- Function Definitions --------- */
int32_t main(int32_t var_0, char **var_1) {
  return allscale::runtime::main_wrapper<__wi_main_work>(var_0, var_1);
}

struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12;
typedef struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12;

struct allscale_type_82;
typedef struct allscale_type_82 allscale_type_82;

template <typename T>
struct is_allscale_fixed_sized_array : public std::false_type {};

template <typename T> struct to_std_array_type;

namespace allscale {
namespace utils {
template <typename T>
struct serializer<T, typename std::enable_if<
                         is_allscale_fixed_sized_array<T>::value, void>::type> {
  using array_t = typename to_std_array_type<T>::type;
  static T load(ArchiveReader &a) {
    return *reinterpret_cast<T *>(&serializer<array_t>::load(a)[0]);
  }
  static void store(ArchiveWriter &a, const T &value) {
    serializer<array_t>::store(a, reinterpret_cast<const array_t &>(value));
  }
};
} // namespace utils
} // namespace allscale

struct allscale_type_82 {
  char data[14];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_82> : public std::true_type {
};
template <> struct to_std_array_type<allscale_type_82> {
  using type = std::array<char, 14u>;
};

using data_item_type_1 = allscale::api::user::data::StaticBalancedBinaryTree<
    allscale::utils::Vector<float, 7>, 29>;
// REGISTER_DATAITEMSERVER_DECLARATION(data_item_type_1)
// REGISTER_DATAITEMSERVER(data_item_type_1)
struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43;
typedef struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43;

struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 {
  std::vector<allscale::utils::Vector<float, 7>> capture_0;
  float capture_1;
  allscale::runtime::DataItemReference<
      allscale::api::user::data::StaticBalancedBinaryTree<
          allscale::utils::Vector<float, 7>, 29>>
      capture_2;
  ;
  void operator()(uint64_t p2) const;
  void store(allscale::utils::ArchiveWriter &p2) const;
  ;
  ;
  static IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43
  load(allscale::utils::ArchiveReader &var_0);
};

typedef std::chrono::time_point<
    std::chrono::system_clock,
    std::chrono::duration<int64_t, std::ratio<1, 1000000000>>>
allscale_type_17();

typedef allscale_type_17 *allscale_type_20;

struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13;
typedef struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13;

struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13 {
  operator allscale_type_20() const;
  ;
  ;
  std::chrono::time_point<
      std::chrono::system_clock,
      std::chrono::duration<int64_t, std::ratio<1, 1000000000>>>
  operator()() const;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13
  load(allscale::utils::ArchiveReader &var_0);
};

struct allscale_type_53;
typedef struct allscale_type_53 allscale_type_53;

struct allscale_type_53 {
  char data[18];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_53> : public std::true_type {
};
template <> struct to_std_array_type<allscale_type_53> {
  using type = std::array<char, 18u>;
};

struct
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12 {
  ;
  ;
  int64_t operator()(
      std::chrono::time_point<
          std::chrono::system_clock,
          std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> const &p2,
      std::chrono::time_point<
          std::chrono::system_clock,
          std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> const &p3)
      const;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12
  load(allscale::utils::ArchiveReader &var_0);
};

struct allscale_type_93;
typedef struct allscale_type_93 allscale_type_93;

struct allscale_type_93 {
  char data[4];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_93> : public std::true_type {
};
template <> struct to_std_array_type<allscale_type_93> {
  using type = std::array<char, 4u>;
};

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long;
typedef struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long;

struct allscale_type_156;
typedef struct allscale_type_156 allscale_type_156;

struct allscale_type_156 {
  char data[33];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_156>
    : public std::true_type {};
template <> struct to_std_array_type<allscale_type_156> {
  using type = std::array<char, 33u>;
};

struct allscale_type_157;
typedef struct allscale_type_157 allscale_type_157;

struct allscale_type_157 {
  char data[7];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_157>
    : public std::true_type {};
template <> struct to_std_array_type<allscale_type_157> {
  using type = std::array<char, 7u>;
};

struct allscale_type_75;
typedef struct allscale_type_75 allscale_type_75;

struct allscale_type_75 {
  char data[2];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_75> : public std::true_type {
};
template <> struct to_std_array_type<allscale_type_75> {
  using type = std::array<char, 2u>;
};

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long;
typedef struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long;

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long;
typedef struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long;

struct DummyCtorParamType;
typedef struct DummyCtorParamType DummyCtorParamType;

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long {
  uint64_t _begin;
  uint64_t _end;
  ;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long();
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
          &p2) = default;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
          &&p2) = default;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long(
      uint64_t const &p2, uint64_t const &p3);
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long(
      uint64_t &&p2, uint64_t &&p3, DummyCtorParamType p4);
  ~IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long() =
      default;
  bool IMP_empty_returns_bool() const;
  uint64_t IMP_size_returns_size_t() const;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
      &
      operator=(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
              &p2) = default;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
      &
      operator=(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
              &&p2) = default;
  uint64_t const &begin_returns_constunsignedlong() const;
  bool covers_returns_bool(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
          &p2) const;
  uint64_t const &end_returns_constunsignedlong() const;
  void
  forEach__Insieme__lambda_homeherbertcodingcallscale_compilertestdata_requirementsbinary_treetwo_point_correlationtwo_point_correlationcpp_528_43_returns_void(
      IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 const
          &p2) const;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
  grow_returns_allscaleapiuseralgorithmdetailrangeunsignedlong(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
          &p2,
      int32_t p3) const;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
  shrink_returns_allscaleapiuseralgorithmdetailrangeunsignedlong(
      int32_t p2) const;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long
  split_returns_fragmentsunsignedlong(uint64_t p2) const;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
  load(allscale::utils::ArchiveReader &var_0);
};

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long;
typedef struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long;

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long {
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
      _range;
  allscale::treeture<void> handle;
  uint64_t depth;
  ;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long();
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
          &p2);
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long const
          &p2) = default;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
          &&p2) = default;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
          &p2,
      allscale::treeture<void> const &p3, uint64_t p4);
  ~IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long() =
      default;
  operator allscale::treeture<void>() const;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
      &
      operator=(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long const
              &p2) = default;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
      &
      operator=(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
              &&p2) = default;
  uint64_t getDepth_returns_stdsize_t() const;
  allscale::treeture<void> const &
  getHandle_returns_constcoretask_reference() const;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
  getLeft_returns_iteration_referenceunsignedlong() const;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
      &
      getRange_returns_constrangeunsignedlong() const;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
  getRight_returns_iteration_referenceunsignedlong() const;
  void wait_returns_void() const;
};

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long
    : public IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long {
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long();
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long
          &&p2) = default;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long(
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
          &p2,
      allscale::treeture<void> &&p3);
  ~IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long();
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long
      &
      operator=(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long
              &&p2) = default;
};

struct
    IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3;
typedef struct
    IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3;

struct
    IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 {
  uint64_t depth;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
      range;
  ;
  ;
  ;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3
  load(allscale::utils::ArchiveReader &var_0);
};

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_SubDependencies_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies;
typedef struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_SubDependencies_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_SubDependencies_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies;

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies;
typedef struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies;

struct __wi_allscale_wi_3_variant_0;
typedef struct __wi_allscale_wi_3_variant_0 __wi_allscale_wi_3_variant_0;

struct __wi_allscale_wi_3_can_split;
typedef struct __wi_allscale_wi_3_can_split __wi_allscale_wi_3_can_split;

struct __wi_allscale_wi_3_variant_1;
typedef struct __wi_allscale_wi_3_variant_1 __wi_allscale_wi_3_variant_1;

struct __wi_allscale_wi_3_name {
  static const char *name() { return "__wi_allscale_wi_3"; }
};

using __wi_allscale_wi_3_work = allscale::work_item_description<
    void, __wi_allscale_wi_3_name, allscale::do_serialization,
    __wi_allscale_wi_3_variant_0, __wi_allscale_wi_3_variant_1,
    __wi_allscale_wi_3_can_split>;

/* ------- Function Definitions --------- */
allscale::treeture<void> allscale_fun_459(
    allscale::runtime::dependencies const &var_0,
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_1) {
  return allscale::spawn_first_with_dependencies<__wi_allscale_wi_3_work>(
      var_0, hpx::util::get<0>(var_1), hpx::util::get<1>(var_1));
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long
allscale_fun_438(
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
        &var_0,
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 const
        &var_1,
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies const
        &var_2) {
  return {
      var_0, allscale::runtime::make_prec_operation<
                 IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3, void>((hpx::util::tuple<IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const &)INS_INIT(
                                                                                                                                                                                                                                                          hpx::util::tuple<
                                                                                                                                                                                                                                                              IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43>){var_1},
                                                                                                                                                                                                                                                      &allscale_fun_459)(
                 (IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
                      &)
                     INS_INIT(
                         IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3){
                         (uint64_t)0, var_0})};
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_pfor_unsigned_space_long__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies_returns_detail_colon__colon_loop_reference_lt_unsigned_space_long_gt_(
    uint64_t const &var_0, uint64_t const &var_1,
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 const
        &var_2,
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies const
        &var_3) {
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long &&)
      allscale_fun_438(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long{
              var_0, var_1},
          var_2, var_3);
}
struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency;
typedef struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency;

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency {
  ;
  ;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency
  load(allscale::utils::ArchiveReader &var_0);
};

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies
    : public IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency {
  ;
  ;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_SubDependencies_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies
  split() const;
};

/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_pfor_unsigned_space_long__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_returns_detail_colon__colon_loop_reference_lt_unsigned_space_long_gt_(
    uint64_t const &var_0, uint64_t const &var_1,
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 const
        &var_2) {
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long &&)
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_pfor_unsigned_space_long__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies_returns_detail_colon__colon_loop_reference_lt_unsigned_space_long_gt_(
          var_0, var_1, var_2,
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies{});
}
struct IMP_Args_instance1;
typedef struct IMP_Args_instance1 IMP_Args_instance1;

struct IMP_Args_instance1 {
  allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> node;
  allscale::utils::Vector<float, 7> min;
  allscale::utils::Vector<float, 7> max;
  ;
  ;
  ;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP_Args_instance1 load(allscale::utils::ArchiveReader &var_0);
};

struct __wi_allscale_wi_1_variant_0;
typedef struct __wi_allscale_wi_1_variant_0 __wi_allscale_wi_1_variant_0;

struct __wi_allscale_wi_1_variant_1;
typedef struct __wi_allscale_wi_1_variant_1 __wi_allscale_wi_1_variant_1;

struct __wi_allscale_wi_1_can_split;
typedef struct __wi_allscale_wi_1_can_split __wi_allscale_wi_1_can_split;

struct __wi_allscale_wi_1_name {
  static const char *name() { return "__wi_allscale_wi_1"; }
};

using __wi_allscale_wi_1_work = allscale::work_item_description<
    void, __wi_allscale_wi_1_name, allscale::do_serialization,
    __wi_allscale_wi_1_variant_0, __wi_allscale_wi_1_variant_1,
    __wi_allscale_wi_1_can_split>;

/* ------- Function Definitions --------- */
allscale::treeture<void> allscale_fun_119(
    allscale::runtime::dependencies const &var_0,
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_1) {
  return allscale::spawn_first_with_dependencies<__wi_allscale_wi_1_work>(
      var_0, hpx::util::get<0>(var_1), hpx::util::get<1>(var_1));
}
/* ------- Function Definitions --------- */
void allscale_fun_98(allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>> &var_0,
                     allscale::utils::Vector<float, 7> const &var_1,
                     allscale::utils::Vector<float, 7> const &var_2) {
  allscale::runtime::make_prec_operation<IMP_Args_instance1, void>(
      (hpx::util::tuple<allscale::runtime::DataItemReference<
           allscale::api::user::data::StaticBalancedBinaryTree<
               allscale::utils::Vector<float, 7>, 29>>> const &)
          INS_INIT(hpx::util::tuple<allscale::runtime::DataItemReference<
                       allscale::api::user::data::StaticBalancedBinaryTree<
                           allscale::utils::Vector<float, 7>, 29>>>){
              (allscale::runtime::DataItemReference<
                  allscale::api::user::data::StaticBalancedBinaryTree<
                      allscale::utils::Vector<float, 7>, 29>> const &)var_0},
      &allscale_fun_119)(
      (IMP_Args_instance1 const &)INS_INIT(IMP_Args_instance1){
          (allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<
               29> &&)
              allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<
                  29>{},
          var_1, var_2})
      .get_result();
}
/* ------- Function Definitions --------- */
void IMP_fill_7_29_returns_void(
    allscale::runtime::DataItemReference<
        allscale::api::user::data::StaticBalancedBinaryTree<
            allscale::utils::Vector<float, 7>, 29>> &var_0,
    float var_1, float var_2) {
  allscale::utils::Vector<float, 7> var_3{(var_1)};
  allscale::utils::Vector<float, 7> var_4{(var_2)};
  allscale_fun_98(var_0, var_3, var_4);
}
struct allscale_type_77;
typedef struct allscale_type_77 allscale_type_77;

struct allscale_type_77 {
  char data[16];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_77> : public std::true_type {
};
template <> struct to_std_array_type<allscale_type_77> {
  using type = std::array<char, 16u>;
};

struct allscale_type_479;
typedef struct allscale_type_479 allscale_type_479;

struct allscale_type_479 {
  char data[15];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_479>
    : public std::true_type {};
template <> struct to_std_array_type<allscale_type_479> {
  using type = std::array<char, 15u>;
};

struct allscale_type_478;
typedef struct allscale_type_478 allscale_type_478;

struct allscale_type_478 {
  char data[6];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_478>
    : public std::true_type {};
template <> struct to_std_array_type<allscale_type_478> {
  using type = std::array<char, 6u>;
};

struct allscale_type_481;
typedef struct allscale_type_481 allscale_type_481;

struct allscale_type_481 {
  char data[29];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_481>
    : public std::true_type {};
template <> struct to_std_array_type<allscale_type_481> {
  using type = std::array<char, 29u>;
};

struct allscale_type_73;
typedef struct allscale_type_73 allscale_type_73;

struct allscale_type_73 {
  char data[30];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_73> : public std::true_type {
};
template <> struct to_std_array_type<allscale_type_73> {
  using type = std::array<char, 30u>;
};

struct allscale_type_74;
typedef struct allscale_type_74 allscale_type_74;

struct allscale_type_74 {
  char data[13];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_74> : public std::true_type {
};
template <> struct to_std_array_type<allscale_type_74> {
  using type = std::array<char, 13u>;
};

struct allscale_type_480;
typedef struct allscale_type_480 allscale_type_480;

struct allscale_type_480 {
  char data[12];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_480>
    : public std::true_type {};
template <> struct to_std_array_type<allscale_type_480> {
  using type = std::array<char, 12u>;
};

struct allscale_type_76;
typedef struct allscale_type_76 allscale_type_76;

struct allscale_type_76 {
  char data[3];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_76> : public std::true_type {
};
template <> struct to_std_array_type<allscale_type_76> {
  using type = std::array<char, 3u>;
};

/* ------- Function Definitions --------- */
allscale::utils::Vector<float, 7> IMP_getRandomPoint_7_returns_Point_lt_7UL_gt_(
    allscale::utils::Vector<float, 7> const &var_0,
    allscale::utils::Vector<float, 7> const &var_1) {
  allscale::utils::Vector<float, 7> var_2;
  {
    for (uint32_t var_3 = (uint32_t)0; var_3 < (uint32_t)7ul; ++var_3) {
      var_2.operator[]((uint64_t)var_3) =
          var_0.operator[]((uint64_t)var_3) +
          (float)rand() / (float)2147483647 *
              (var_1.operator[]((uint64_t)var_3) -
               var_0.operator[]((uint64_t)var_3));
    };
  };
  return (allscale::utils::Vector<float, 7> &&) var_2;
}
/* ------- Function Definitions --------- */
int32_t allscale_fun_5(int32_t var_0, char **var_1) {
  const int32_t var_2 = 7;
  const uint64_t var_3 = (uint64_t)29;
  const uint64_t var_4 = (uint64_t)1000;
  const float var_5 = (float)0;
  const float var_6 = (float)100;
  const float var_7 = (float)20;
  IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13 var_8 = (IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13 &&)
      INS_INIT(
          IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13){};
  IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12 var_9 =
      (IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12 &&) INS_INIT(IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12){};
  std::chrono::time_point<
      std::chrono::system_clock,
      std::chrono::duration<int64_t, std::ratio<1, 1000000000>>>
      var_10 =
          (std::chrono::time_point<
               std::chrono::system_clock,
               std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> &&)
              var_8.
              operator()();
  std::chrono::time_point<
      std::chrono::system_clock,
      std::chrono::duration<int64_t, std::ratio<1, 1000000000>>>
      var_11 = (std::chrono::time_point<
                std::chrono::system_clock,
                std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> const
                    &)var_10;
  std::cout << "Two-Point-Correlation with 2^" << var_3 << " points in ["
            << var_5 << "," << var_6 << ")^" << var_2
            << " space, radius=" << var_7 << "\n";
  std::cout << "Creating ... " << &std::flush;
  var_10 = var_8.operator()();
  allscale::runtime::DataItemReference<
      allscale::api::user::data::StaticBalancedBinaryTree<
          allscale::utils::Vector<float, 7>, 29>>
      var_12 = allscale::runtime::DataItemManager::create<
          allscale::api::user::data::StaticBalancedBinaryTree<
              allscale::utils::Vector<float, 7>, 29>>();
  var_11 = var_8.operator()();
  std::cout << std::setw(6) << var_9.operator()(var_10, var_11) << "ms\n";
  std::cout << "Filling  ... " << &std::flush;
  var_10 = var_8.operator()();
  IMP_fill_7_29_returns_void(var_12, var_5, var_6);
  var_11 = var_8.operator()();
  std::cout << std::setw(6) << var_9.operator()(var_10, var_11) << "ms\n";
  std::cout << "\n\n ----- Benchmarking -----\n\n";
  std::cout << "Creating benchmark input set (M=" << var_4 << ") ... "
            << &std::flush;
  var_10 = var_8.operator()();
  std::vector<allscale::utils::Vector<float, 7>> var_13;
  {
    for (uint64_t var_14 = (uint64_t)0; var_14 < var_4; ++var_14) {
      var_13.push_back(IMP_getRandomPoint_7_returns_Point_lt_7UL_gt_(
          allscale::utils::Vector<float, 7>{var_5},
          allscale::utils::Vector<float, 7>{var_6}));
    };
  };
  var_11 = var_8.operator()();
  std::cout << std::setw(6) << var_9.operator()(var_10, var_11) << "ms\n";
  for (int i = 0; i < 3; i++) {
    std::cout << "Benchmarking ... " << &std::flush;

    var_10 = var_8.operator()();
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_pfor_unsigned_space_long__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_returns_detail_colon__colon_loop_reference_lt_unsigned_space_long_gt_(
        (uint64_t)0, var_13.size(),
        INS_INIT(
            IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43){
            (std::vector<allscale::utils::Vector<float, 7>> const &)var_13,
            var_7,
            (allscale::runtime::DataItemReference<
                allscale::api::user::data::StaticBalancedBinaryTree<
                    allscale::utils::Vector<float, 7>, 29>> const &)var_12});
    var_11 = var_8.operator()();
    int64_t var_15 = var_9.operator()(var_10, var_11);
    std::cout << std::setw(22) << var_15 << "ms - ";
    std::cout << (uint64_t)var_15 / var_4 << "ms per query, "
              << var_4 * (uint64_t)1000 / (uint64_t)var_15 << " queries/s\n";
  }
  std::cout << "\n ------------------------\n\n";
  return 0;
}
ALLSCALE_REGISTER_TREETURE_TYPE(int32_t)
/* ------- Function Definitions --------- */
allscale::treeture<int32_t>
allscale_fun_3(hpx::util::tuple<int32_t, char **> const &var_0) {
  return allscale::treeture<int32_t>(
      allscale_fun_5(hpx::util::get<0>(var_0), hpx::util::get<1>(var_0)));
}
struct __wi_main_variant_0 {
  static allscale::treeture<int32_t>
  execute(hpx::util::tuple<int32_t, char **> const &var_0);
  static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
int64_t
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12::
operator()(
    std::chrono::time_point<
        std::chrono::system_clock,
        std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> const &var_1,
    std::chrono::time_point<
        std::chrono::system_clock,
        std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> const &var_2)
    const {
  if (getenv("NO_TIME_IN_OUTPUT") != (char *)0) {
    return (int64_t)-1;
  };
  return std::chrono::duration_cast<
             std::chrono::duration<int64_t, std::ratio<1, 1000>>, int64_t,
             std::ratio<1, 1000000000>>((var_2 - var_1))
      .count();
}
/* ------- Function Definitions --------- */
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12::
    load(allscale::utils::ArchiveReader &var_0) {
  return {};
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
void IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12_long_const_space_type_minus_parameter_minus_0_minus_0_space__ampersand__const_space_type_minus_parameter_minus_0_minus_1_space__ampersand__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_421_12::
    store(allscale::utils::ArchiveWriter &var_1) const {}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
void IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43::
    store(allscale::utils::ArchiveWriter &var_1) const {
  var_1.write<std::vector<allscale::utils::Vector<float, 7>>>(
      (*this).capture_0);
  var_1.write<float>((*this).capture_1);
  var_1.write<allscale::runtime::DataItemReference<
      allscale::api::user::data::StaticBalancedBinaryTree<
          allscale::utils::Vector<float, 7>, 29>>>((*this).capture_2);
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
ALLSCALE_REGISTER_TREETURE_TYPE(uint64_t)
struct IMP_BoundingBox_7;
typedef struct IMP_BoundingBox_7 IMP_BoundingBox_7;

struct IMP_BoundingBox_7 {
  allscale::utils::Vector<float, 7> low;
  allscale::utils::Vector<float, 7> high;
  ;
  IMP_BoundingBox_7();
  IMP_BoundingBox_7(IMP_BoundingBox_7 const &p2) = default;
  IMP_BoundingBox_7(IMP_BoundingBox_7 &&p2) = default;
  IMP_BoundingBox_7(allscale::utils::Vector<float, 7> &&p2,
                    allscale::utils::Vector<float, 7> &&p3,
                    DummyCtorParamType p4);
  ~IMP_BoundingBox_7() = default;
  IMP_BoundingBox_7 &operator=(IMP_BoundingBox_7 const &p2) = default;
  IMP_BoundingBox_7 &operator=(IMP_BoundingBox_7 &&p2) = default;
  bool contains_returns_bool(allscale::utils::Vector<float, 7> const &p2) const;
  float minimumDistanceSquared_returns_distance_t(
      allscale::utils::Vector<float, 7> const &p2) const;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP_BoundingBox_7 load(allscale::utils::ArchiveReader &var_0);
};

struct IMP_Args;
typedef struct IMP_Args IMP_Args;

struct IMP_Args {
  allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> node;
  IMP_BoundingBox_7 box;
  ;
  ;
  ;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP_Args load(allscale::utils::ArchiveReader &var_0);
};

struct __wi_allscale_wi_2_can_split;
typedef struct __wi_allscale_wi_2_can_split __wi_allscale_wi_2_can_split;

struct __wi_allscale_wi_2_variant_1;
typedef struct __wi_allscale_wi_2_variant_1 __wi_allscale_wi_2_variant_1;

struct __wi_allscale_wi_2_variant_0;
typedef struct __wi_allscale_wi_2_variant_0 __wi_allscale_wi_2_variant_0;

struct __wi_allscale_wi_2_name {
  static const char *name() { return "__wi_allscale_wi_2"; }
};

using __wi_allscale_wi_2_work = allscale::work_item_description<
    uint64_t, __wi_allscale_wi_2_name, allscale::do_serialization,
    __wi_allscale_wi_2_variant_0, __wi_allscale_wi_2_variant_1,
    __wi_allscale_wi_2_can_split>;

/* ------- Function Definitions --------- */
allscale::treeture<uint64_t> allscale_fun_242(
    allscale::runtime::dependencies const &var_0,
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_1) {
  return allscale::spawn_first_with_dependencies<__wi_allscale_wi_2_work>(
      var_0, hpx::util::get<0>(var_1), hpx::util::get<1>(var_1),
      hpx::util::get<2>(var_1), hpx::util::get<3>(var_1));
}
/* ------- Function Definitions --------- */
uint64_t IMP_twoPointCorrelation_7_29_returns_std_colon__colon_size_t(
    allscale::runtime::DataItemReference<
        allscale::api::user::data::StaticBalancedBinaryTree<
            allscale::utils::Vector<float, 7>, 29>> const &var_0,
    allscale::utils::Vector<float, 7> const &var_1, float var_2) {
  float var_3 = var_2 * var_2;
  IMP_BoundingBox_7 var_4;
  return allscale::runtime::make_prec_operation<IMP_Args, uint64_t>(
             (hpx::util::tuple<
                 allscale::runtime::DataItemReference<
                     allscale::api::user::data::StaticBalancedBinaryTree<
                         allscale::utils::Vector<float, 7>, 29>>,
                 allscale::utils::Vector<float, 7>, float> const &)
                 INS_INIT(hpx::util::tuple<
                          allscale::runtime::DataItemReference<
                              allscale::api::user::data::
                                  StaticBalancedBinaryTree<
                                      allscale::utils::Vector<float, 7>, 29>>,
                          allscale::utils::Vector<float, 7>, float>){
                     var_0, var_1, var_3},
             &allscale_fun_242)(
             (IMP_Args const &)INS_INIT(IMP_Args){
                 (allscale::api::user::data::
                      StaticBalancedBinaryTreeElementAddress<29> &&)
                     allscale::api::user::data::
                         StaticBalancedBinaryTreeElementAddress<29>{},
                 (IMP_BoundingBox_7 const &)var_4})
      .get_result();
}
/* ------- Function Definitions --------- */
void IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43::
operator()(uint64_t pos) const {
  IMP_twoPointCorrelation_7_29_returns_std_colon__colon_size_t(
      (allscale::runtime::DataItemReference<
          allscale::api::user::data::StaticBalancedBinaryTree<
              allscale::utils::Vector<float, 7>, 29>> &)(*this)
          .capture_2,
      (*this).capture_0.operator[](pos), (*this).capture_1);
}
struct DummyCtorParamType {
  ;
  ;
};

/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
bool IMP_BoundingBox_7::contains_returns_bool(
    allscale::utils::Vector<float, 7> const &var_1) const {
  return (*this).low.dominatedBy(var_1) &&
         var_1.strictlyDominatedBy((*this).high);
}
/* ------- Function Definitions --------- */
void IMP_BoundingBox_7::store(allscale::utils::ArchiveWriter &var_1) const {
  var_1.write<allscale::utils::Vector<float, 7>>((*this).low);
  var_1.write<allscale::utils::Vector<float, 7>>((*this).high);
}
/* ------- Function Definitions --------- */
float IMP_allscale_colon__colon_utils_colon__colon_sumOfSquares_float_7_returns_float(
    allscale::utils::Vector<float, 7> const &var_0) {
  float var_1 = 0.0f;
  {
    for (uint32_t var_2 = (uint32_t)0; var_2 < (uint32_t)7ul; ++var_2) {
      var_1 +=
          var_0.operator[]((uint64_t)var_2) * var_0.operator[]((uint64_t)var_2);
    };
  };
  return var_1;
}
/* ------- Function Definitions --------- */
float IMP_lengthSquared_7_returns_distance_t(
    allscale::utils::Vector<float, 7> const &var_0) {
  return IMP_allscale_colon__colon_utils_colon__colon_sumOfSquares_float_7_returns_float(
      var_0);
}
/* ------- Function Definitions --------- */
float IMP_BoundingBox_7::minimumDistanceSquared_returns_distance_t(
    allscale::utils::Vector<float, 7> const &var_1) const {
  allscale::utils::Vector<float, 7> var_2;
  {
    for (uint32_t var_3 = (uint32_t)0; var_3 < (uint32_t)7ul; ++var_3) {
      var_2.operator[]((uint64_t)var_3) =
          std::max((*this).low.operator[]((uint64_t)var_3) -
                       var_1.operator[]((uint64_t)var_3),
                   std::max(var_1.operator[]((uint64_t)var_3) -
                                (*this).high.operator[]((uint64_t)var_3),
                            (float)0));
    };
  };
  return IMP_lengthSquared_7_returns_distance_t(var_2);
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_BoundingBox_7
IMP_BoundingBox_7::load(allscale::utils::ArchiveReader &var_0) {
  allscale::utils::Vector<float, 7> var_1 =
      var_0.read<allscale::utils::Vector<float, 7>>();
  allscale::utils::Vector<float, 7> var_2 =
      var_0.read<allscale::utils::Vector<float, 7>>();
  DummyCtorParamType var_3;
  return {std::move(var_1), std::move(var_2), var_3};
}
/* ------- Function Definitions --------- */
IMP_BoundingBox_7::IMP_BoundingBox_7(allscale::utils::Vector<float, 7> &&var_1,
                                     allscale::utils::Vector<float, 7> &&var_2,
                                     DummyCtorParamType var_3)
    : low(var_1), high(var_2) {}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_BoundingBox_7::IMP_BoundingBox_7() : low(), high() {
  float var_1 = std::numeric_limits<float>::max();
  {
    for (uint32_t var_2 = (uint32_t)0; var_2 < (uint32_t)7ul; ++var_2) {
      {
        (*this).low.operator[]((uint64_t)var_2) = 0.0f - var_1;
        (*this).high.operator[]((uint64_t)var_2) = var_1;
      };
    };
  };
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_Args IMP_Args::load(allscale::utils::ArchiveReader &var_0) {
  allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> var_1 =
      var_0.read<allscale::api::user::data::
                     StaticBalancedBinaryTreeElementAddress<29>>();
  IMP_BoundingBox_7 var_2 = var_0.read<IMP_BoundingBox_7>();
  return {std::move(var_1), std::move(var_2)};
}
/* ------- Function Definitions --------- */
void IMP_Args::store(allscale::utils::ArchiveWriter &var_1) const {
  var_1.write<
      allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29>>(
      (*this).node);
  var_1.write<IMP_BoundingBox_7>((*this).box);
}
/* ------- Function Definitions --------- */
bool allscale_fun_258(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return ((IMP_Args const &)hpx::util::get<0>(var_0)).node.isLeaf() ||
         ((IMP_Args const &)hpx::util::get<0>(var_0)).node.getLevel() >
             (int32_t)((uint64_t)2 * 29ul / (uint64_t)3);
}
/* ------- Function Definitions --------- */
bool allscale_fun_264(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return !allscale_fun_258(var_0);
}
struct __wi_allscale_wi_2_can_split {
  static bool
  call(hpx::util::tuple<IMP_Args,
                        allscale::runtime::DataItemReference<
                            allscale::api::user::data::StaticBalancedBinaryTree<
                                allscale::utils::Vector<float, 7>, 29>>,
                        allscale::utils::Vector<float, 7>, float> const &var_0);
};

bool __wi_allscale_wi_2_can_split::call(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return allscale_fun_264(var_0);
}
/* ------- Function Definitions --------- */
hpx::util::tuple<allscale::runtime::DataItemRequirement<
    allscale::api::user::data::StaticBalancedBinaryTree<
        allscale::utils::Vector<float, 7>, 29>>>
allscale_fun_263(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return INS_INIT(hpx::util::tuple<allscale::runtime::DataItemRequirement<
                      allscale::api::user::data::StaticBalancedBinaryTree<
                          allscale::utils::Vector<float, 7>, 29>>>){
      allscale::runtime::createDataItemRequirement(
          (allscale::runtime::DataItemReference<
              allscale::api::user::data::StaticBalancedBinaryTree<
                  allscale::utils::Vector<float, 7>, 29>> const &)
              hpx::util::get<1>(var_0),
          allscale::api::user::data::StaticBalancedBinaryTreeRegion<
              29>::closure(hpx::util::get<0>(var_0).node.getSubtreeIndex()),
          allscale::runtime::AccessMode::ReadWrite)};
}
/* ------- Function Prototypes ---------- */
uint64_t
IMP__lparen_anonymous_space_namespace_rparen__colon__colon_twoPointCorrelationSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_298_2_7_29_returns_std_colon__colon_size_t(
    allscale::runtime::DataItemReference<
        allscale::api::user::data::StaticBalancedBinaryTree<
            allscale::utils::Vector<float, 7>, 29>> const &p1,
    allscale::utils::Vector<float, 7> const &p2, float p3,
    allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> const
        &p4,
    IMP_BoundingBox_7 const &p5);

/* ------- Function Definitions --------- */
uint64_t allscale_fun_260(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return IMP__lparen_anonymous_space_namespace_rparen__colon__colon_twoPointCorrelationSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_298_2_7_29_returns_std_colon__colon_size_t(
      (allscale::runtime::DataItemReference<
          allscale::api::user::data::StaticBalancedBinaryTree<
              allscale::utils::Vector<float, 7>, 29>> const &)
          hpx::util::get<1>(var_0),
      hpx::util::get<2>(var_0), hpx::util::get<3>(var_0),
      ((IMP_Args const &)hpx::util::get<0>(var_0)).node,
      ((IMP_Args const &)hpx::util::get<0>(var_0)).box);
}
/* ------- Function Definitions --------- */
uint64_t allscale_fun_262(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return IMP__lparen_anonymous_space_namespace_rparen__colon__colon_twoPointCorrelationSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_298_2_7_29_returns_std_colon__colon_size_t(
      (allscale::runtime::DataItemReference<
          allscale::api::user::data::StaticBalancedBinaryTree<
              allscale::utils::Vector<float, 7>, 29>> const &)
          hpx::util::get<1>(var_0),
      hpx::util::get<2>(var_0), hpx::util::get<3>(var_0),
      ((IMP_Args const &)hpx::util::get<0>(var_0)).node,
      ((IMP_Args const &)hpx::util::get<0>(var_0)).box);
}
/* ------- Function Definitions --------- */
uint64_t allscale_fun_256(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  if (allscale_fun_258(var_0)) {
    return allscale_fun_260(var_0);
  } else {
    return allscale_fun_262(var_0);
  };
}
/* ------- Function Definitions --------- */
allscale::treeture<uint64_t> allscale_fun_255(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return allscale::treeture<uint64_t>(allscale_fun_256(var_0));
}
struct __wi_allscale_wi_2_variant_1 {
  static allscale::treeture<uint64_t> execute(
      hpx::util::tuple<IMP_Args,
                       allscale::runtime::DataItemReference<
                           allscale::api::user::data::StaticBalancedBinaryTree<
                               allscale::utils::Vector<float, 7>, 29>>,
                       allscale::utils::Vector<float, 7>, float> const &var_0);
  static hpx::util::tuple<allscale::runtime::DataItemRequirement<
      allscale::api::user::data::StaticBalancedBinaryTree<
          allscale::utils::Vector<float, 7>, 29>>>
  get_requirements(
      hpx::util::tuple<IMP_Args,
                       allscale::runtime::DataItemReference<
                           allscale::api::user::data::StaticBalancedBinaryTree<
                               allscale::utils::Vector<float, 7>, 29>>,
                       allscale::utils::Vector<float, 7>, float> const &var_0);
  static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
allscale::utils::Vector<float, 7> &
IMP_allscale_colon__colon_utils_colon__colon__operator_minus_assign_(
    allscale::utils::Vector<float, 7> &var_0,
    allscale::utils::Vector<float, 7> const &var_1) {
  {
    for (uint64_t var_2 = (uint64_t)0; var_2 < 7ul; ++var_2) {
      var_0.operator[](var_2) -= var_1.operator[](var_2);
    };
  };
  return var_0;
}
/* ------- Function Definitions --------- */
allscale::utils::Vector<float, 7>
IMP_allscale_colon__colon_utils_colon__colon__operator_minus_(
    allscale::utils::Vector<float, 7> const &var_0,
    allscale::utils::Vector<float, 7> const &var_1) {
  allscale::utils::Vector<float, 7> var_2 = var_0;
  return (allscale::utils::Vector<float, 7> const &)
      IMP_allscale_colon__colon_utils_colon__colon__operator_minus_assign_(
          var_2, var_1);
}
/* ------- Function Definitions --------- */
float IMP_distanceSquared_7_returns_distance_t(
    allscale::utils::Vector<float, 7> const &var_0,
    allscale::utils::Vector<float, 7> const &var_1) {
  return IMP_allscale_colon__colon_utils_colon__colon_sumOfSquares_float_7_returns_float(
      IMP_allscale_colon__colon_utils_colon__colon__operator_minus_(var_0,
                                                                    var_1));
}
/* ------- Function Definitions --------- */
uint64_t
IMP__lparen_anonymous_space_namespace_rparen__colon__colon_twoPointCorrelationSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_298_2_7_29_returns_std_colon__colon_size_t(
    allscale::runtime::DataItemReference<
        allscale::api::user::data::StaticBalancedBinaryTree<
            allscale::utils::Vector<float, 7>, 29>> const &var_0,
    allscale::utils::Vector<float, 7> const &var_1, float var_2,
    allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> const
        &var_3,
    IMP_BoundingBox_7 const &var_4) {
  allscale::api::user::data::StaticBalancedBinaryTree<
      allscale::utils::Vector<float, 7>, 29> const &&var_5 =
      allscale::runtime::DataItemManager::get(var_0);
  if (var_4.minimumDistanceSquared_returns_distance_t(var_1) > var_2) {
    return (uint64_t)0;
  };
  allscale::utils::Vector<float, 7> const &var_6 = var_5.operator[](var_3);
  uint64_t var_7 = (uint64_t)0;
  if (IMP_distanceSquared_7_returns_distance_t(var_1, var_6) <= var_2) {
    var_7++;
  };
  if (var_3.isLeaf()) {
    return var_7;
  };
  IMP_BoundingBox_7 var_8 = var_4;
  IMP_BoundingBox_7 var_9 = var_4;
  uint64_t var_10 = (uint64_t)var_3.getLevel() % 7ul;
  var_8.high.operator[](var_10) = var_6.operator[](var_10);
  var_9.low.operator[](var_10) = var_6.operator[](var_10);
  return var_7 +
         IMP__lparen_anonymous_space_namespace_rparen__colon__colon_twoPointCorrelationSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_298_2_7_29_returns_std_colon__colon_size_t(
             var_0, var_1, var_2, var_3.getLeftChild(), var_8) +
         IMP__lparen_anonymous_space_namespace_rparen__colon__colon_twoPointCorrelationSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_298_2_7_29_returns_std_colon__colon_size_t(
             var_0, var_1, var_2, var_3.getRightChild(), var_9);
}
hpx::util::tuple<allscale::runtime::DataItemRequirement<
    allscale::api::user::data::StaticBalancedBinaryTree<
        allscale::utils::Vector<float, 7>, 29>>>
__wi_allscale_wi_2_variant_1::get_requirements(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return allscale_fun_263(var_0);
}
allscale::treeture<uint64_t> __wi_allscale_wi_2_variant_1::execute(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return allscale_fun_255(var_0);
}
/* ------- Function Definitions --------- */
hpx::util::tuple<allscale::runtime::DataItemRequirement<
    allscale::api::user::data::StaticBalancedBinaryTree<
        allscale::utils::Vector<float, 7>, 29>>>
allscale_fun_253(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return INS_INIT(hpx::util::tuple<allscale::runtime::DataItemRequirement<
                      allscale::api::user::data::StaticBalancedBinaryTree<
                          allscale::utils::Vector<float, 7>, 29>>>){
      allscale::runtime::createDataItemRequirement(
          (allscale::runtime::DataItemReference<
              allscale::api::user::data::StaticBalancedBinaryTree<
                  allscale::utils::Vector<float, 7>, 29>> const &)
              hpx::util::get<1>(var_0),
          allscale::api::user::data::StaticBalancedBinaryTreeRegion<
              29>::subtree(hpx::util::get<0>(var_0).node.getSubtreeIndex()),
          allscale::runtime::AccessMode::ReadWrite)};
}
/* ------- Function Definitions --------- */
allscale::treeture<uint64_t> allscale_fun_246(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  allscale::api::user::data::StaticBalancedBinaryTree<
      allscale::utils::Vector<float, 7>, 29> const &&var_1 =
      allscale::runtime::DataItemManager::get(
          (allscale::runtime::DataItemReference<
              allscale::api::user::data::StaticBalancedBinaryTree<
                  allscale::utils::Vector<float, 7>, 29>> const &)
              hpx::util::get<1>(var_0));
  allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> const
      &var_2 = ((IMP_Args const &)hpx::util::get<0>(var_0)).node;
  IMP_BoundingBox_7 const &var_3 =
      ((IMP_Args const &)hpx::util::get<0>(var_0)).box;
  if (var_3.minimumDistanceSquared_returns_distance_t(
          hpx::util::get<2>(var_0)) > hpx::util::get<3>(var_0)) {
    return allscale::treeture<uint64_t>((uint64_t)0);
  };
  allscale::utils::Vector<float, 7> const &var_4 = var_1.operator[](var_2);
  uint64_t var_5 = (uint64_t)0;
  if (IMP_distanceSquared_7_returns_distance_t(
          hpx::util::get<2>(var_0), var_4) < hpx::util::get<3>(var_0)) {
    var_5++;
  };
  if (var_2.isLeaf()) {
    return allscale::treeture<uint64_t>(var_5);
  };
  IMP_BoundingBox_7 var_6 = var_3;
  IMP_BoundingBox_7 var_7 = var_3;
  uint64_t var_8 = (uint64_t)var_2.getLevel() % 7ul;
  var_6.high.operator[](var_8) = var_4.operator[](var_8);
  var_7.low.operator[](var_8) = var_4.operator[](var_8);
  allscale::treeture<uint64_t> var_9 =
      allscale::spawn_with_dependencies<__wi_allscale_wi_2_work>(
          allscale::runtime::after(),
          (IMP_Args const &)INS_INIT(IMP_Args){
              (allscale::api::user::data::
                   StaticBalancedBinaryTreeElementAddress<29> &&)
                  var_2.getLeftChild(),
              (IMP_BoundingBox_7 const &)var_6},
          (allscale::runtime::DataItemReference<
              allscale::api::user::data::StaticBalancedBinaryTree<
                  allscale::utils::Vector<float, 7>, 29>> const &)
              hpx::util::get<1>(var_0),
          (allscale::utils::Vector<float, 7> const &)hpx::util::get<2>(var_0),
          (float const &)hpx::util::get<3>(var_0));
  allscale::treeture<uint64_t> var_10 =
      allscale::spawn_with_dependencies<__wi_allscale_wi_2_work>(
          allscale::runtime::after(),
          (IMP_Args const &)INS_INIT(IMP_Args){
              (allscale::api::user::data::
                   StaticBalancedBinaryTreeElementAddress<29> &&)
                  var_2.getRightChild(),
              (IMP_BoundingBox_7 const &)var_7},
          (allscale::runtime::DataItemReference<
              allscale::api::user::data::StaticBalancedBinaryTree<
                  allscale::utils::Vector<float, 7>, 29>> const &)
              hpx::util::get<1>(var_0),
          (allscale::utils::Vector<float, 7> const &)hpx::util::get<2>(var_0),
          (float const &)hpx::util::get<3>(var_0));
  return allscale::treeture<uint64_t>(var_5 + var_9.get_result() +
                                      var_10.get_result());
}
/* ------- Function Definitions --------- */
allscale::treeture<uint64_t> allscale_fun_244(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return allscale_fun_246(var_0);
}
struct __wi_allscale_wi_2_variant_0 {
  static allscale::treeture<uint64_t> execute(
      hpx::util::tuple<IMP_Args,
                       allscale::runtime::DataItemReference<
                           allscale::api::user::data::StaticBalancedBinaryTree<
                               allscale::utils::Vector<float, 7>, 29>>,
                       allscale::utils::Vector<float, 7>, float> const &var_0);
  static hpx::util::tuple<allscale::runtime::DataItemRequirement<
      allscale::api::user::data::StaticBalancedBinaryTree<
          allscale::utils::Vector<float, 7>, 29>>>
  get_requirements(
      hpx::util::tuple<IMP_Args,
                       allscale::runtime::DataItemReference<
                           allscale::api::user::data::StaticBalancedBinaryTree<
                               allscale::utils::Vector<float, 7>, 29>>,
                       allscale::utils::Vector<float, 7>, float> const &var_0);
  static constexpr bool valid = true;
};

hpx::util::tuple<allscale::runtime::DataItemRequirement<
    allscale::api::user::data::StaticBalancedBinaryTree<
        allscale::utils::Vector<float, 7>, 29>>>
__wi_allscale_wi_2_variant_0::get_requirements(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return allscale_fun_253(var_0);
}
allscale::treeture<uint64_t> __wi_allscale_wi_2_variant_0::execute(
    hpx::util::tuple<IMP_Args,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>,
                     allscale::utils::Vector<float, 7>, float> const &var_0) {
  return allscale_fun_244(var_0);
}
/* ------- Function Definitions --------- */
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43::
    load(allscale::utils::ArchiveReader &var_0) {
  std::vector<allscale::utils::Vector<float, 7>> var_1 =
      var_0.read<std::vector<allscale::utils::Vector<float, 7>>>();
  float var_2 = var_0.read<float>();
  allscale::runtime::DataItemReference<
      allscale::api::user::data::StaticBalancedBinaryTree<
          allscale::utils::Vector<float, 7>, 29>>
      var_3 = var_0.read<allscale::runtime::DataItemReference<
          allscale::api::user::data::StaticBalancedBinaryTree<
              allscale::utils::Vector<float, 7>, 29>>>();
  return {std::move(var_1), std::move(var_2), std::move(var_3)};
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
std::chrono::time_point<
    std::chrono::system_clock,
    std::chrono::duration<int64_t, std::ratio<1, 1000000000>>>
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13::
operator()() const {
  return (std::chrono::time_point<
              std::chrono::system_clock,
              std::chrono::duration<int64_t, std::ratio<1, 1000000000>>> &&)
      std::chrono::system_clock::now();
}
/* ------- Function Definitions --------- */
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13::
    load(allscale::utils::ArchiveReader &var_0) {
  return {};
}
/* ------- Function Definitions --------- */
IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13::
operator allscale_type_20() const {
  assert((bool)false && (bool)"This is an Insieme generated dummy function "
                              "which should never be called");
}
/* ------- Function Definitions --------- */
void IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13_struct_space_std_colon__colon_chrono_colon__colon_time_point_lt_struct_space_std_colon__colon_chrono_colon__colon__V2_colon__colon_system_clock_comma__space_struct_space_std_colon__colon_chrono_colon__colon_duration_lt_long_comma__space_struct_space_std_colon__colon_ratio_lt_1_comma__space_1000000000_gt__space__gt__space__gt__IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_420_13::
    store(allscale::utils::ArchiveWriter &var_1) const {}
struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long {
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
      left;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
      right;
  ;
};

/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long()
    : _begin(0ul), _end(0ul) {}
/* ------- Function Definitions --------- */
bool IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_covers_unsigned_space_long_returns_bool(
    uint64_t const &var_0, uint64_t const &var_1, uint64_t const &var_2,
    uint64_t const &var_3) {
  return var_2 >= var_3 || (var_0 <= var_2 && var_3 <= var_1);
}
/* ------- Function Definitions --------- */
bool IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    covers_returns_bool(
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
            &var_1) const {
  return IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_covers_unsigned_space_long_returns_bool(
      (*this)._begin, (*this)._end, var_1._begin, var_1._end);
}
/* ------- Function Definitions --------- */
bool IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    IMP_empty_returns_bool() const {
  return (*this).IMP_size_returns_size_t() == (uint64_t)0;
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long(
        uint64_t const &var_1, uint64_t const &var_2)
    : _begin(var_1), _end(var_2) {
  if ((*this).IMP_empty_returns_bool()) {
    (*this)._end = (*this)._begin;
  };
}
struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool;
typedef struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool;

struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool {
  ;
  ;
  uint64_t operator()(uint64_t p2, uint64_t p3) const;
  void store(allscale::utils::ArchiveWriter &p2) const;
  static IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool
  load(allscale::utils::ArchiveReader &var_0);
};

/* ------- Function Definitions --------- */
uint64_t
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    IMP_size_returns_size_t() const {
  return IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool{}
      .
      operator()((*this)._begin, (*this)._end);
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool::
    load(allscale::utils::ArchiveReader &var_0) {
  return {};
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
uint64_t
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool::
operator()(uint64_t pos, uint64_t var_2) const {
  return pos < var_2 ? var_2 - pos : (uint64_t)0;
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
void IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_volume_unsigned_space_long_bool::
    store(allscale::utils::ArchiveWriter &var_1) const {}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    shrink_returns_allscaleapiuseralgorithmdetailrangeunsignedlong(
        int32_t var_1) const {
  return (IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long &&)(
             *this)
      .grow_returns_allscaleapiuseralgorithmdetailrangeunsignedlong(*this,
                                                                    0 - var_1);
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long(
        uint64_t &&var_1, uint64_t &&var_2, DummyCtorParamType var_3)
    : _begin(var_1), _end(var_2) {}
/* ------- Function Definitions --------- */
uint64_t const &
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    end_returns_constunsignedlong() const {
  return (*this)._end;
}
/* ------- Function Definitions --------- */
uint64_t
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_access_unsigned_space_long_returns_typename_space_std_colon__colon_enable_if_lt_std_colon__colon_is_integral_lt_unsigned_space_long_gt__colon__colon_value_comma__space_unsigned_space_long_gt__colon__colon_type(
    uint64_t var_0) {
  return var_0;
}
/* ------- Function Definitions --------- */
void IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_forEach_unsigned_space_long__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_returns_void(
    uint64_t const &var_0, uint64_t const &var_1,
    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 const
        &var_2) {
  for (uint64_t var_3 = var_0; var_3 < var_1; ++var_3) {
    var_2.operator()(
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_access_unsigned_space_long_returns_typename_space_std_colon__colon_enable_if_lt_std_colon__colon_is_integral_lt_unsigned_space_long_gt__colon__colon_value_comma__space_unsigned_space_long_gt__colon__colon_type(
            var_3));
  };
}
/* ------- Function Definitions --------- */
void IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    forEach__Insieme__lambda_homeherbertcodingcallscale_compilertestdata_requirementsbinary_treetwo_point_correlationtwo_point_correlationcpp_528_43_returns_void(
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 const
            &var_1) const {
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_forEach_unsigned_space_long__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_returns_void(
      (*this)._begin, (*this)._end, var_1);
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    load(allscale::utils::ArchiveReader &var_0) {
  uint64_t pos = var_0.read<uint64_t>();
  uint64_t var_2 = var_0.read<uint64_t>();
  DummyCtorParamType var_3;
  return {std::move(pos), std::move(var_2), var_3};
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_make_fragments_unsigned_space_long_returns_fragments_lt_unsigned_space_long_gt_(
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
        &var_0,
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
        &var_1) {
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long &&)
      INS_INIT(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long){
          var_0, var_1};
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_spliter_unsigned_space_long__static__IMP_split_returns_fragments_lt_unsigned_space_long_gt_(
    uint64_t var_0,
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
        &var_1) {
  uint64_t const &var_2 = var_1.begin_returns_constunsignedlong();
  uint64_t const &var_3 = var_1.end_returns_constunsignedlong();
  uint64_t var_4 = var_2 + (var_3 - var_2) / (uint64_t)2;
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long &&)
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_make_fragments_unsigned_space_long_returns_fragments_lt_unsigned_space_long_gt_(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long{
              var_2, var_4},
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long{
              var_4, var_3});
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    split_returns_fragmentsunsignedlong(uint64_t pos) const {
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long &&)
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_spliter_unsigned_space_long__static__IMP_split_returns_fragments_lt_unsigned_space_long_gt_(
          pos, *this);
}
/* ------- Function Definitions --------- */
uint64_t const &
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    begin_returns_constunsignedlong() const {
  return (*this)._begin;
}
/* ------- Function Definitions --------- */
void IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    store(allscale::utils::ArchiveWriter &var_1) const {
  var_1.write<uint64_t>((*this)._begin);
  var_1.write<uint64_t>((*this)._end);
}
/* ------- Function Definitions --------- */
uint64_t
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_grow_unsigned_space_long_returns_unsigned_space_long(
    uint64_t const &var_0, uint64_t const &var_1, int32_t var_2) {
  return std::min(var_1, var_0 + (uint64_t)var_2);
}
/* ------- Function Definitions --------- */
uint64_t
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_shrink_unsigned_space_long_returns_unsigned_space_long(
    uint64_t const &var_0, uint64_t const &var_1, int32_t var_2) {
  return std::max(var_1, var_0 - (uint64_t)var_2);
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long::
    grow_returns_allscaleapiuseralgorithmdetailrangeunsignedlong(
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
            &var_1,
        int32_t var_2) const {
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long &&)
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long{
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_shrink_unsigned_space_long_returns_unsigned_space_long(
              (*this)._begin, var_1.begin_returns_constunsignedlong(), var_2),
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_grow_unsigned_space_long_returns_unsigned_space_long(
              (*this)._end, var_1.end_returns_constunsignedlong(), var_2)};
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
uint64_t
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    getDepth_returns_stdsize_t() const {
  return (*this).depth;
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
            &var_1,
        allscale::treeture<void> const &var_2, uint64_t var_3)
    : _range(var_1), handle(var_2), depth(var_3) {}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
operator allscale::treeture<void>() const {
  return (*this).handle;
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long
allscale_fun_382(
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
        &var_0,
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
        &var_1) {
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long &&)
      INS_INIT(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long){
          var_0, var_1};
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long
allscale_fun_380(
    uint64_t var_0,
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
        &var_1) {
  uint64_t const &var_2 = var_1.begin_returns_constunsignedlong();
  uint64_t const &var_3 = var_1.end_returns_constunsignedlong();
  uint64_t var_4 = var_2 + (var_3 - var_2) / (uint64_t)2;
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long &&)
      allscale_fun_382(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long{
              var_2, var_4},
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long{
              var_4, var_3});
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    getRight_returns_iteration_referenceunsignedlong() const {
  return {allscale_fun_380((*this).depth, (*this)._range).right,
          (*this).handle.get_right_child(), (*this).depth + (uint64_t)1};
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const &
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    getRange_returns_constrangeunsignedlong() const {
  return (*this)._range;
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long()
    : IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long{}) {
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
            &var_1)
    : _range(var_1), handle(allscale::make_ready_treeture()),
      depth((uint64_t)0) {}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    getLeft_returns_iteration_referenceunsignedlong() const {
  return {allscale_fun_380((*this).depth, (*this)._range).left,
          (*this).handle.get_left_child(), (*this).depth + (uint64_t)1};
}
/* ------- Function Definitions --------- */
allscale::treeture<void> const &
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    getHandle_returns_constcoretask_reference() const {
  return (*this).handle;
}
/* ------- Function Definitions --------- */
void IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long::
    wait_returns_void() const {
  if ((*this).handle.valid()) {
    (*this).handle.wait();
  };
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long::
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long(
        IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
            &var_1,
        allscale::treeture<void> &&var_2)
    : IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
          var_1, std::move(var_2), (uint64_t)0) {}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long::
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long()
    : IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long(
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long{}) {
}
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long::
    ~IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_reference_unsigned_space_long() {
  (*(IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_iteration_reference_unsigned_space_long
         *)this)
      .wait_returns_void();
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
void IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3::
    store(allscale::utils::ArchiveWriter &var_1) const {
  var_1.write<uint64_t>((*this).depth);
  var_1.write<
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long>(
      (*this).range);
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3
IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3::
    load(allscale::utils::ArchiveReader &var_0) {
  uint64_t pos = var_0.read<uint64_t>();
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long
      var_2 = var_0.read<
          IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long>();
  return {std::move(pos), std::move(var_2)};
}
struct
    IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_SubDependencies_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies {
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies
      left;
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies
      right;
  ;
};

/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency::
    load(allscale::utils::ArchiveReader &var_0) {
  return {};
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
void IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_loop_dependency::
    store(allscale::utils::ArchiveWriter &var_1) const {}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_SubDependencies_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies
IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies::
    split() const {
  return (
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_SubDependencies_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies &&)
      IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_SubDependencies_struct_space_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_no_dependencies{};
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
allscale::treeture<void> allscale_fun_463(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long
      var_1 =
          (IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_fragments_unsigned_space_long &&)(
              (IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
                   &)hpx::util::get<0>(var_0))
              .range.split_returns_fragmentsunsignedlong(
                  ((IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
                        &)hpx::util::get<0>(var_0))
                      .depth);
  return allscale::runtime::treeture_parallel(allscale::runtime::after(),
                                              std::
                                                  move(allscale::spawn_with_dependencies<
                                                       __wi_allscale_wi_3_work>(allscale::runtime::
                                                                                    after(),
                                                                                (IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
                                                                                     &)
                                                                                    INS_INIT(
                                                                                        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3){
                                                                                        (
                                                                                            (
                                                                                                IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
                                                                                                    &)
                                                                                                hpx::util::get<
                                                                                                    0>(var_0))
                                                                                                .depth +
                                                                                            (uint64_t)1,
                                                                                        (
                                                                                            IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const
                                                                                                &)
                                                                                            var_1
                                                                                                .left},
                                                                                (
                                                                                    IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 const
                                                                                        &)hpx::util::get<1>(var_0))),
                                              std::
                                                  move(
                                                      allscale::spawn_with_dependencies<__wi_allscale_wi_3_work>(allscale::
                                                                                                                     runtime::after(),
                                                                                                                 (IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
                                                                                                                      &)
                                                                                                                     INS_INIT(IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3){((
                                                                                                                                                                                                                                                                                                                                                                  IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const &)hpx::
                                                                                                                                                                                                                                                                                                                                                                  util::get<0>(var_0))
                                                                                                                                                                                                                                                                                                                                                                     .depth +
                                                                                                                                                                                                                                                                                                                                                                 (uint64_t)1,
                                                                                                                                                                                                                                                                                                                                                             (IMP_allscale_colon__colon_api_colon__colon_user_colon__colon_algorithm_colon__colon_detail_colon__colon_range_unsigned_space_long const &)
                                                                                                                                                                                                                                                                                                                                                                 var_1
                                                                                                                                                                                                                                                                                                                                                                     .right},
                                                                                                                 (IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43 const &)
                                                                                                                     hpx::util::get<
                                                                                                                         1>(
                                                                                                                         var_0))));
}
/* ------- Function Definitions --------- */
allscale::treeture<void> allscale_fun_461(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  return allscale_fun_463(var_0);
}
struct __wi_allscale_wi_3_variant_0 {
  static allscale::treeture<void>
  execute(hpx::util::tuple<
          IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
          IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
              &var_0);
  static hpx::util::tuple<> get_requirements(
      hpx::util::tuple<
          IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
          IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
          &var_0) {
    return {};
  }
  static constexpr bool valid = true;
};

allscale::treeture<void> __wi_allscale_wi_3_variant_0::execute(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  return allscale_fun_461(var_0);
}
/* ------- Function Definitions --------- */
bool allscale_fun_470(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  return ((IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
               &)hpx::util::get<0>(var_0))
             .range.IMP_size_returns_size_t() <= (uint64_t)1;
}
/* ------- Function Definitions --------- */
bool allscale_fun_474(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  return !allscale_fun_470(var_0);
}
struct __wi_allscale_wi_3_can_split {
  static bool
  call(hpx::util::tuple<
       IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
       IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
           &var_0);
};

bool __wi_allscale_wi_3_can_split::call(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  return allscale_fun_474(var_0);
}
/* ------- Function Definitions --------- */
void allscale_fun_473(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  ((IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
        &)hpx::util::get<0>(var_0))
      .range
      .forEach__Insieme__lambda_homeherbertcodingcallscale_compilertestdata_requirementsbinary_treetwo_point_correlationtwo_point_correlationcpp_528_43_returns_void(
          hpx::util::get<1>(var_0));
  return (void)0;
}
/* ------- Function Definitions --------- */
void allscale_fun_472(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  ((IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3 const
        &)hpx::util::get<0>(var_0))
      .range
      .forEach__Insieme__lambda_homeherbertcodingcallscale_compilertestdata_requirementsbinary_treetwo_point_correlationtwo_point_correlationcpp_528_43_returns_void(
          hpx::util::get<1>(var_0));
}
/* ------- Function Definitions --------- */
void allscale_fun_468(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  if (allscale_fun_470(var_0)) {
    return allscale_fun_472(var_0);
  } else {
    return allscale_fun_473(var_0);
  };
}
/* ------- Function Definitions --------- */
allscale::runtime::unused_type allscale_fun_466(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  allscale_fun_468(var_0);
  return {};
}
struct __wi_allscale_wi_3_variant_1 {
  static allscale::runtime::unused_type
  execute(hpx::util::tuple<
          IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
          IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
              &var_0);
  static hpx::util::tuple<> get_requirements(
      hpx::util::tuple<
          IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
          IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
          &var_0) {
    return {};
  }
  static constexpr bool valid = true;
};

allscale::runtime::unused_type __wi_allscale_wi_3_variant_1::execute(
    hpx::util::tuple<
        IMP_RecArgs_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_api_slash_code_slash_api_slash_include_slash_allscale_slash_api_slash_user_slash_algorithm_slash_pfor_dot_h_1130_3,
        IMP__Insieme__lambda__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43_std_colon__colon_vector_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_std_colon__colon_allocator_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__space__gt__space__gt__const_space_float_allscale_colon__colon_api_colon__colon_user_colon__colon_data_colon__colon_StaticBalancedBinaryTree_lt_allscale_colon__colon_utils_colon__colon_Vector_lt_float_comma__space_7_gt__comma__space_29_gt__space__ampersand__void_unsigned_space_long_IMLOC__slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_528_43> const
        &var_0) {
  return allscale_fun_466(var_0);
}
/* ------- Function Definitions --------- */
IMP_Args_instance1
IMP_Args_instance1::load(allscale::utils::ArchiveReader &var_0) {
  allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> var_1 =
      var_0.read<allscale::api::user::data::
                     StaticBalancedBinaryTreeElementAddress<29>>();
  allscale::utils::Vector<float, 7> var_2 =
      var_0.read<allscale::utils::Vector<float, 7>>();
  allscale::utils::Vector<float, 7> var_3 =
      var_0.read<allscale::utils::Vector<float, 7>>();
  return {std::move(var_1), std::move(var_2), std::move(var_3)};
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
void IMP_Args_instance1::store(allscale::utils::ArchiveWriter &var_1) const {
  var_1.write<
      allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29>>(
      (*this).node);
  var_1.write<allscale::utils::Vector<float, 7>>((*this).min);
  var_1.write<allscale::utils::Vector<float, 7>>((*this).max);
}
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
hpx::util::tuple<allscale::runtime::DataItemRequirement<
    allscale::api::user::data::StaticBalancedBinaryTree<
        allscale::utils::Vector<float, 7>, 29>>>
allscale_fun_142(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return hpx::util::make_tuple(allscale::runtime::createDataItemRequirement(
      hpx::util::get<1>(var_0),
      allscale::api::user::data::StaticBalancedBinaryTreeRegion<29>::subtree(
          hpx::util::get<0>(var_0).node.getSubtreeIndex()),
      allscale::runtime::AccessMode::ReadWrite));
}
struct allscale_type_140;
typedef struct allscale_type_140 allscale_type_140;

struct allscale_type_140 {
  char data[231];
  ;
};

template <>
struct is_allscale_fixed_sized_array<allscale_type_140>
    : public std::true_type {};
template <> struct to_std_array_type<allscale_type_140> {
  using type = std::array<char, 231u>;
};

/* ------- Function Definitions --------- */
allscale::treeture<void> allscale_fun_123(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  allscale::api::user::data::StaticBalancedBinaryTree<
      allscale::utils::Vector<float, 7>, 29> &&var_1 =
      allscale::runtime::DataItemManager::get(hpx::util::get<1>(var_0));

  uint64_t pos = hpx::util::get<0>(var_0).node.getLevel() % 7ul;

  allscale::utils::Vector<float, 7> &var_3 =
      var_1[hpx::util::get<0>(var_0).node];

  var_3 = ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0)).min;
  var_3.operator[](pos) =
      ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0))
          .min.
          operator[](pos) +
      (((IMP_Args_instance1 const &)hpx::util::get<0>(var_0))
           .max.
           operator[](pos) -
       ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0))
           .min.
           operator[](pos)) /
          (float)2;
  allscale::utils::Vector<float, 7> var_5 = hpx::util::get<0>(var_0).max;
  allscale::utils::Vector<float, 7> var_6 = hpx::util::get<0>(var_0).min;
  var_5.operator[](pos) = var_3.operator[](pos);
  var_6.operator[](pos) = var_3.operator[](pos);
  return allscale::runtime::treeture_parallel(
      allscale::runtime::after(),
      std::move(allscale::spawn_with_dependencies<__wi_allscale_wi_1_work>(
          allscale::runtime::after(),
          (IMP_Args_instance1 const &)INS_INIT(IMP_Args_instance1){
              (allscale::api::user::data::
                   StaticBalancedBinaryTreeElementAddress<29> &&)(
                  (IMP_Args_instance1 const &)hpx::util::get<0>(var_0))
                  .node.getLeftChild(),
              (allscale::utils::Vector<float, 7> const
                   &)((IMP_Args_instance1 const &)hpx::util::get<0>(var_0))
                  .min,
              (allscale::utils::Vector<float, 7> const &)var_5},
          (allscale::runtime::DataItemReference<
              allscale::api::user::data::StaticBalancedBinaryTree<
                  allscale::utils::Vector<float, 7>, 29>> const &)
              hpx::util::get<1>(var_0))),
      std::move(allscale::spawn_with_dependencies<__wi_allscale_wi_1_work>(
          allscale::runtime::after(),
          (IMP_Args_instance1 const &)INS_INIT(IMP_Args_instance1){
              (allscale::api::user::data::
                   StaticBalancedBinaryTreeElementAddress<29> &&)(
                  (IMP_Args_instance1 const &)hpx::util::get<0>(var_0))
                  .node.getRightChild(),
              (allscale::utils::Vector<float, 7> const &)var_6,
              (allscale::utils::Vector<float, 7> const
                   &)((IMP_Args_instance1 const &)hpx::util::get<0>(var_0))
                  .max},
          (allscale::runtime::DataItemReference<
              allscale::api::user::data::StaticBalancedBinaryTree<
                  allscale::utils::Vector<float, 7>, 29>> const &)
              hpx::util::get<1>(var_0))));
}
/* ------- Function Definitions --------- */
allscale::treeture<void> allscale_fun_121(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return allscale_fun_123(var_0);
}
struct __wi_allscale_wi_1_variant_0 {
  static allscale::treeture<void> execute(
      hpx::util::tuple<IMP_Args_instance1,
                       allscale::runtime::DataItemReference<
                           allscale::api::user::data::StaticBalancedBinaryTree<
                               allscale::utils::Vector<float, 7>, 29>>> const
          &var_0);
  static hpx::util::tuple<allscale::runtime::DataItemRequirement<
      allscale::api::user::data::StaticBalancedBinaryTree<
          allscale::utils::Vector<float, 7>, 29>>>
  get_requirements(
      hpx::util::tuple<IMP_Args_instance1,
                       allscale::runtime::DataItemReference<
                           allscale::api::user::data::StaticBalancedBinaryTree<
                               allscale::utils::Vector<float, 7>, 29>>> const
          &var_0);
  static constexpr bool valid = true;
};

hpx::util::tuple<allscale::runtime::DataItemRequirement<
    allscale::api::user::data::StaticBalancedBinaryTree<
        allscale::utils::Vector<float, 7>, 29>>>
__wi_allscale_wi_1_variant_0::get_requirements(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return allscale_fun_142(var_0);
}
allscale::treeture<void> __wi_allscale_wi_1_variant_0::execute(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return allscale_fun_121(var_0);
}
/* ------- Function Definitions --------- */
bool allscale_fun_148(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0)).node.isLeaf();
}
/* ------- Function Prototypes ---------- */
void IMP__lparen_anonymous_space_namespace_rparen__colon__colon_fillSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_100_2_7_29_returns_void(
    allscale::runtime::DataItemReference<
        allscale::api::user::data::StaticBalancedBinaryTree<
            allscale::utils::Vector<float, 7>, 29>> &p1,
    allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> const
        &p2,
    allscale::utils::Vector<float, 7> const &p3,
    allscale::utils::Vector<float, 7> const &p4);

/* ------- Function Definitions --------- */
void allscale_fun_152(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  IMP__lparen_anonymous_space_namespace_rparen__colon__colon_fillSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_100_2_7_29_returns_void(
      (allscale::runtime::DataItemReference<
          allscale::api::user::data::StaticBalancedBinaryTree<
              allscale::utils::Vector<float, 7>, 29>> &)
          hpx::util::get<1>(var_0),
      ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0)).node,
      ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0)).min,
      ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0)).max);
  return (void)0;
}
/* ------- Function Definitions --------- */
void allscale_fun_150(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  IMP__lparen_anonymous_space_namespace_rparen__colon__colon_fillSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_100_2_7_29_returns_void(
      (allscale::runtime::DataItemReference<
          allscale::api::user::data::StaticBalancedBinaryTree<
              allscale::utils::Vector<float, 7>, 29>> &)
          hpx::util::get<1>(var_0),
      ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0)).node,
      ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0)).min,
      ((IMP_Args_instance1 const &)hpx::util::get<0>(var_0)).max);
}
/* ------- Function Definitions --------- */
void rec(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  if (allscale_fun_148(var_0)) {
    return allscale_fun_150(var_0);
  } else {
    return allscale_fun_152(var_0);
  };
}
/* ------- Function Definitions --------- */
allscale::runtime::unused_type allscale_fun_145(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  rec(var_0);
  return {};
}
/* ------- Function Definitions --------- */
hpx::util::tuple<allscale::runtime::DataItemRequirement<
    allscale::api::user::data::StaticBalancedBinaryTree<
        allscale::utils::Vector<float, 7>, 29>>>
allscale_fun_153(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
    return hpx::util::make_tuple(
      allscale::runtime::createDataItemRequirement(
              hpx::util::get<1>(var_0),
          allscale::api::user::data::StaticBalancedBinaryTreeRegion<
              29>::closure(hpx::util::get<0>(var_0).node.getSubtreeIndex()),
          allscale::runtime::AccessMode::ReadWrite));
}
struct __wi_allscale_wi_1_variant_1 {
  static allscale::runtime::unused_type execute(
      hpx::util::tuple<IMP_Args_instance1,
                       allscale::runtime::DataItemReference<
                           allscale::api::user::data::StaticBalancedBinaryTree<
                               allscale::utils::Vector<float, 7>, 29>>> const
          &var_0);
  static hpx::util::tuple<allscale::runtime::DataItemRequirement<
      allscale::api::user::data::StaticBalancedBinaryTree<
          allscale::utils::Vector<float, 7>, 29>>>
  get_requirements(
      hpx::util::tuple<IMP_Args_instance1,
                       allscale::runtime::DataItemReference<
                           allscale::api::user::data::StaticBalancedBinaryTree<
                               allscale::utils::Vector<float, 7>, 29>>> const
          &var_0);
  static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
void IMP__lparen_anonymous_space_namespace_rparen__colon__colon_fillSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_100_2_7_29_returns_void(
    allscale::runtime::DataItemReference<
        allscale::api::user::data::StaticBalancedBinaryTree<
            allscale::utils::Vector<float, 7>, 29>> &var_0,
    allscale::api::user::data::StaticBalancedBinaryTreeElementAddress<29> const
        &addr,
    allscale::utils::Vector<float, 7> const &var_2,
    allscale::utils::Vector<float, 7> const &var_3) {
  allscale::api::user::data::StaticBalancedBinaryTree<
      allscale::utils::Vector<float, 7>, 29> &&var_4 =
      allscale::runtime::DataItemManager::get(var_0);
  uint64_t var_5 = (uint64_t)addr.getLevel() % 7ul;
  allscale::utils::Vector<float, 7> &var_6 = var_4.operator[](addr);
  var_6 = var_2;
  var_6.operator[](var_5) =
      var_2.operator[](var_5) +
      (var_3.operator[](var_5) - var_2.operator[](var_5)) / (float)2;
  if (addr.isLeaf()) {
    return;
  };
  allscale::utils::Vector<float, 7> var_7 = var_3;
  allscale::utils::Vector<float, 7> var_8 = var_2;
  var_7.operator[](var_5) = var_6.operator[](var_5);
  var_8.operator[](var_5) = var_6.operator[](var_5);
  IMP__lparen_anonymous_space_namespace_rparen__colon__colon_fillSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_100_2_7_29_returns_void(
      var_0, addr.getLeftChild(), var_2, var_7);
  IMP__lparen_anonymous_space_namespace_rparen__colon__colon_fillSequential_slash_home_slash_herbert_slash_coding_slash_c_plus__plus__slash_allscale_compiler_slash_test_slash_data_requirements_slash_binary_tree_slash_two_point_correlation_slash_two_point_correlation_dot_cpp_100_2_7_29_returns_void(
      var_0, addr.getRightChild(), var_8, var_3);
}
allscale::runtime::unused_type __wi_allscale_wi_1_variant_1::execute(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return allscale_fun_145(var_0);
}
hpx::util::tuple<allscale::runtime::DataItemRequirement<
    allscale::api::user::data::StaticBalancedBinaryTree<
        allscale::utils::Vector<float, 7>, 29>>>
__wi_allscale_wi_1_variant_1::get_requirements(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return allscale_fun_153(var_0);
}
/* ------- Function Definitions --------- */
bool allscale_fun_154(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return !allscale_fun_148(var_0);
}
struct __wi_allscale_wi_1_can_split {
  static bool
  call(hpx::util::tuple<IMP_Args_instance1,
                        allscale::runtime::DataItemReference<
                            allscale::api::user::data::StaticBalancedBinaryTree<
                                allscale::utils::Vector<float, 7>, 29>>> const
           &var_0);
};

bool __wi_allscale_wi_1_can_split::call(
    hpx::util::tuple<IMP_Args_instance1,
                     allscale::runtime::DataItemReference<
                         allscale::api::user::data::StaticBalancedBinaryTree<
                             allscale::utils::Vector<float, 7>, 29>>> const
        &var_0) {
  return allscale_fun_154(var_0);
}
allscale::treeture<int32_t>
__wi_main_variant_0::execute(hpx::util::tuple<int32_t, char **> const &var_0) {
  return allscale_fun_3(var_0);
}
/* ------- Function Definitions --------- */
bool allscale_fun_482(hpx::util::tuple<int32_t, char **> const &var_0) {
  return (bool)false;
}
struct __wi_main_can_split {
  static bool call(hpx::util::tuple<int32_t, char **> const &var_0);
};

bool __wi_main_can_split::call(
    hpx::util::tuple<int32_t, char **> const &var_0) {
  return allscale_fun_482(var_0);
}
struct __wi_main_variant_1 {
  static allscale::treeture<int32_t>
  execute(hpx::util::tuple<int32_t, char **> const &var_0);
  static constexpr bool valid = true;
};

allscale::treeture<int32_t>
__wi_main_variant_1::execute(hpx::util::tuple<int32_t, char **> const &var_0) {
  return allscale_fun_3(var_0);
}
