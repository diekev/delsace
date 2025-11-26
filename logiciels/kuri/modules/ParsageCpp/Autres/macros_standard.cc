#include <stdio.h>

int main(int argc, char **argv)
{
#ifdef __cpp_aligned_new
    printf("ajoute_définition(préprocesseur, \"__cpp_aligned_new\", %ld)\n", __cpp_aligned_new);
#endif
#ifdef __cpp_lib_three_way_comparison
    printf("ajoute_définition(préprocesseur, \"__cpp_lib_three_way_comparison\", %ld)\n", __cpp_lib_three_way_comparison);
#endif
#ifdef __cpp_deduction_guides
    printf("ajoute_définition(préprocesseur, \"__cpp_deduction_guides\", %ld)\n", __cpp_deduction_guides);
#endif
#ifdef __cpp_concepts
    printf("ajoute_définition(préprocesseur, \"__cpp_concepts\", %ld)\n", __cpp_concepts);
#endif
#ifdef __cpp_lib_concepts
    printf("ajoute_définition(préprocesseur, \"__cpp_lib_concepts\", %ld)\n", __cpp_lib_concepts);
#endif
#ifdef __cpp_inline_variables
    printf("ajoute_définition(préprocesseur, \"__cpp_inline_variables\", %ld)\n", __cpp_inline_variables);
#endif
#ifdef __cpp_sized_deallocation
    printf("ajoute_définition(préprocesseur, \"__cpp_sized_deallocation\", %ld)\n", __cpp_sized_deallocation);
#endif
#ifdef __cpp_transactional_memory
    printf("ajoute_définition(préprocesseur, \"__cpp_transactional_memory\", %ld)\n", __cpp_transactional_memory);
#endif
#ifdef __cpp_if_consteval
    printf("ajoute_définition(préprocesseur, \"__cpp_if_consteval\", %ld)\n", __cpp_if_consteval);
#endif
#ifdef __cpp_noexcept_function_type
    printf("ajoute_définition(préprocesseur, \"__cpp_noexcept_function_type\", %ld)\n", __cpp_noexcept_function_type);
#endif
#ifdef __cpp_exceptions
    printf("ajoute_définition(préprocesseur, \"__cpp_exceptions\", %ld)\n", __cpp_exceptions);
#endif
#ifdef __STDC_HOSTED__
    printf("ajoute_définition(préprocesseur, \"__STDC_HOSTED__\", %ld)\n", __STDC_HOSTED__);
#endif
#ifdef __FLT_MANT_DIG__
    printf("ajoute_définition(préprocesseur, \"__FLT_MANT_DIG__\", %ld)\n", __FLT_MANT_DIG__);
#endif
#ifdef __DBL_MANT_DIG__
    printf("ajoute_définition(préprocesseur, \"__DBL_MANT_DIG__\", %ld)\n", __DBL_MANT_DIG__);
#endif
#ifdef __SANITIZE_THREAD__
    printf("ajoute_définition(préprocesseur, \"__SANITIZE_THREAD__\", %ld)\n", __SANITIZE_THREAD__);
#endif
#ifdef __GNUC__
    printf("ajoute_définition(préprocesseur, \"__GNUC__\", %ld)\n", __GNUC__);
#endif
#ifdef __STDCPP_WANT_MATH_SPEC_FUNCS__
    printf("ajoute_définition(préprocesseur, \"__STDCPP_WANT_MATH_SPEC_FUNCS__\", %ld)\n", __STDCPP_WANT_MATH_SPEC_FUNCS__);
#endif
#ifdef _GLIBCXX_HAVE_IS_CONSTANT_EVALUATED
    printf("ajoute_définition(préprocesseur, \"_GLIBCXX_HAVE_IS_CONSTANT_EVALUATED\", %ld)\n", _GLIBCXX_HAVE_IS_CONSTANT_EVALUATED);
#endif
#ifdef _GLIBCXX_USE_C99_CHECK
    printf("ajoute_définition(préprocesseur, \"_GLIBCXX_USE_C99_CHECK\", %ld)\n", _GLIBCXX_USE_C99_CHECK);
#endif
#ifdef _GLIBCXX_USE_C99_DYNAMIC
    printf("ajoute_définition(préprocesseur, \"_GLIBCXX_USE_C99_DYNAMIC\", %ld)\n", _GLIBCXX_USE_C99_DYNAMIC);
#endif
#ifdef _GLIBCXX_USE_C99_LONG_LONG_CHECK
    printf("ajoute_définition(préprocesseur, \"_GLIBCXX_USE_C99_LONG_LONG_CHECK\", %ld)\n", _GLIBCXX_USE_C99_LONG_LONG_CHECK);
#endif
#ifdef _GLIBCXX_USE_C99_LONG_LONG_DYNAMIC
    printf("ajoute_définition(préprocesseur, \"_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC\", %ld)\n", _GLIBCXX_USE_C99_LONG_LONG_DYNAMIC);
#endif
#ifdef __linux__
    printf("ajoute_définition(préprocesseur, \"__linux__\", %ld)\n", __linux__);
#endif
#ifdef __x86_64__
    printf("ajoute_définition(préprocesseur, \"__x86_64__\", %ld)\n", __x86_64__);
#endif
#ifdef __LP64__
    printf("ajoute_définition(préprocesseur, \"__LP64__\", %ld)\n", __LP64__);
#endif
#ifdef __CHAR_BIT__
    printf("ajoute_définition(préprocesseur, \"__CHAR_BIT__\", %ld)\n", __CHAR_BIT__);
#endif
#ifdef __SCHAR_MAX__
    printf("ajoute_définition(préprocesseur, \"__SCHAR_MAX__\", %ld)\n", __SCHAR_MAX__);
#endif
#ifdef __SHRT_MAX__
    printf("ajoute_définition(préprocesseur, \"__SHRT_MAX__\", %ld)\n", __SHRT_MAX__);
#endif
#ifdef __INT_MAX__
    printf("ajoute_définition(préprocesseur, \"__INT_MAX__\", %ld)\n", __INT_MAX__);
#endif
#ifdef __LONG_MAX__
    printf("ajoute_définition(préprocesseur, \"__LONG_MAX__\", %ld)\n", __LONG_MAX__);
#endif
#ifdef __FLT_EVAL_METHOD__
    printf("ajoute_définition(préprocesseur, \"__FLT_EVAL_METHOD__\", %ld)\n", __FLT_EVAL_METHOD__);
#endif
#ifdef __clang_minor__
    printf("ajoute_définition(préprocesseur, \"__clang_minor__\", %ld)\n", __clang_minor__);
#endif
#ifdef __clang_major__
    printf("ajoute_définition(préprocesseur, \"__clang_major__\", %ld)\n", __clang_major__);
#endif
#ifdef __cplusplus
    printf("ajoute_définition(préprocesseur, \"__cplusplus\", %ld)\n", __cplusplus);
#endif
}
