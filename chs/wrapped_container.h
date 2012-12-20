#ifndef wrapped_containerH
#define wrapped_containerH

#include <algorithm>

struct simple_deleter
{
  template <typename T>
  void operator()(const T& p) const { delete p; }
};

template <typename T, typename Deleter = simple_deleter>
struct wrapped_container : public T
{
  typedef T container_type;
  typedef Deleter deleter_type;

  wrapped_container() {}
  wrapped_container(const T& container) : T(container) {}

  virtual ~wrapped_container(void)
  {
    std::for_each(T::begin(), T::end(), deleter_type());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(wrapped_container);
};

#endif

