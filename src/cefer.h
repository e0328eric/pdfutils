#ifndef _CEFER
#define _CEFER

#if __STDC_VERSION__ < 202311L // on c23, bool is introduced
#include <stdbool.h>
#endif

#define __CONCAT__(a, b) a##b

typedef void(*__DeferFn)(void*);
struct __Defer {
    __DeferFn fn;
    void* arg;
    bool* cond;
};
static inline void _defer_invoke(struct __Defer* d) {
    if (d && d->fn && (!d->cond || (d->cond && *d->cond))) d->fn(d->arg);
}

#if defined(__GNUC__) || defined(__clang__)

#define __DEFER(_cond, _func, _arg, _count) \
    __attribute__((cleanup(_defer_invoke))) \
    struct __Defer __CONCAT__(_defer_var_, _count) = { .fn = (_func), .arg = (void*)(_arg), .cond = (_cond) }
#define DEFER_IF(_cond, _func, _arg) __DEFER(_cond, _func, _arg, __COUNTER__)
#define DEFER(_func, _arg)           __DEFER(NULL, _func, _arg, __COUNTER__)

#else
#error "only gcc, and clang are supported"
#endif

#endif // _CEFER


