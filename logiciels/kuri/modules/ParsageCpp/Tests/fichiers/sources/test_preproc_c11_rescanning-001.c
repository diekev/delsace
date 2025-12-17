#define f(a) a*g
#define g(a) f(a)
f(2)(9) // peut-être 2*9*g ou 2*f(9)