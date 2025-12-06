#define PARAMETRES(a, b, c) a + b + c

#if PARAMETRES(0, 0, 1)
#else
#  error "Error"
#endif