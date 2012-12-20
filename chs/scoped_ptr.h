#ifndef scoped_ptrH
#define scoped_ptrH

#include "macros.h"

#include <stdlib.h>

template <typename T>
struct scoped_ptr
{
 public:
  typedef T element_type;

  explicit scoped_ptr(T* ptr = NULL) : ptr(ptr) {}
  ~scoped_ptr() { if (ptr) { delete ptr; } }

  T& operator*() { return *ptr; }
  const T& operator*() const { return *ptr; }

  T* operator->() { return ptr; }
  const T* operator->() const { return ptr; }

  operator bool() { return ptr != NULL; }

 private:
  DISALLOW_COPY_AND_ASSIGN(scoped_ptr);

  T* ptr;
};

#endif

