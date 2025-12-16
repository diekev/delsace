#define __ASSERT_VOID_CAST  (void)
#define assert(expr)  (__ASSERT_VOID_CAST (0))

void foo()
{
    assert(123);
}
