
// #if defined _TIME_BITS
// # if _TIME_BITS == 64
// #  if ! defined (_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64
// #   error "_TIME_BITS=64 is allowed only with _FILE_OFFSET_BITS=64"
// #  elif __TIMESIZE == 32
// #   define __USE_TIME_BITS64	1
// #  endif
// # elif _TIME_BITS == 32
// #  if __TIMESIZE > 32
// #   error "_TIME_BITS=32 is not compatible with __TIMESIZE > 32"
// #  endif
// # else
// #  error Invalid _TIME_BITS value (can only be 32 or 64-bit)
// # endif
// #endif

#if ((!defined _POSIX_C_SOURCE || (_POSIX_C_SOURCE - 0) < 199506)      && (defined _REENTRANT || defined _THREAD_SAFE))
#endif