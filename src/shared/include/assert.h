#ifdef NDEBUG
#define assert(stmt) do {} while (0)
#else
#define assert(stmt) do { if (!(stmt)) __badassert(__func__, __FILE__, __LINE__); } while (0)
#endif

_Noreturn void __badassert(const char *func, const char *file, int line);
