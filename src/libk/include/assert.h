#ifdef NDEBUG
#define assert(stmt) ((void)0)
#else
#define assert(stmt) (void)((stmt) || (__badassert(__func__, __FILE__, __LINE__),0))
#endif

#ifdef __SPARSE
#undef assert
#define assert(stmt) (0)
#endif

_Noreturn void __badassert(const char *func, const char *file, int line);
