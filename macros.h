#ifndef macrosH
#define macrosH

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName)\
    TypeName(const TypeName&);\
    TypeName& operator=(const TypeName&)
#endif

#ifndef DISALLOW_COMPARISON
#define DISALLOW_COMPARISON(TypeName)\
    bool operator==(const TypeName&)\
    bool operator!=(const TypeName&)
#endif

#ifndef DISALLOW_INSTANCE
#define DISALLOW_INSTANCES(TypeName)\
    TypeName();\
    ~TypeName()
#endif


#ifndef NDEBUG
#define assertWarn(E)\
    ((void)(!(E) ?\
            fprintf(stderr,\
                    "%s:%d: Assertion failed: " #E "\n",\
                    __FILE__, __LINE__) :\
            0))
#else
#define assertWarn(E)
#endif


#define inRange(Lo, Hi, Val)            (Lo <= Val && Val <= Hi)
#define assertInRange(Lo, Hi, Val)      assert(Lo <= Val),\
                                        assert(Val <= Hi)
#define assertInRangeWarn(Lo, Hi, Val)  assertWarn(Lo <= Val),\
                                        assertWarn(Val <= Hi)
#define restrictRange(Lo, Hi, Val)      ((Val) < (Lo) ? (Lo) :\
                                          ((Val) > (Hi) ? (Hi) : (Val)))


#ifndef NDEBUG
#include <cstdio>
#include <cstring>
/*namespace {
inline void print_debug(const char* file, int line, const char* func) {
  const char* f = file + strlen(file) - 1;
  while (*f != '/') --f;
  ++f;
  printf("%s:%d %s(): ", f, line, func);
}
}*/
#define LOG   printf("%s:%d %s(): ", __FILE__, __LINE__, __FUNCTION__), printf
#else
#define LOG 
#endif

#endif

