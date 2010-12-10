// vim:filetype=cpp:textwidth=80:shiftwidth=4:softtabstop=4:expandtab

#ifndef macrosH
#define macrosH

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName)\
        TypeName(const TypeName&);\
        TypeName& operator = (const TypeName&)
#endif

#ifndef DISALLOW_COMPARISON
#define DISALLOW_COMPARISON(TypeName)\
        bool operator == (const TypeName&)\
        bool operator != (const TypeName&)
#endif

#ifndef DISALLOW_INSTANCE
#define DISALLOW_INSTANCES(TypeName)\
        TypeName();\
        ~TypeName()
#endif


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

