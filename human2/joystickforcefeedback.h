#ifndef __JOYSTICKFORCEFEEDBACK_H__
#define __JOYSTICKFORCEFEEDBACK_H__

#include <stdint.h>
#include <linux/input.h>
#include <iostream>

class JoystickForceFeedback
{
 public:
  enum Direction {
    DIRECTION_DOWN = 0,
    DIRECTION_LEFT = 16384,
    DIRECTION_UP = 32768,
    DIRECTION_RIGHT = 49152
  };

  class Exception : public std::exception
  {
   public:
    explicit Exception(const char* msg) throw() : msg_(msg) {}
    virtual const char* what() const throw() { return msg_; }
   private:
    const char* msg_;
  };

  static JoystickForceFeedback* create();

  virtual ~JoystickForceFeedback();

  void rumble(uint16_t strong_magnitude = 1000,
              uint16_t weak_magnitude = 0,
              Direction direction = DIRECTION_DOWN,
              uint16_t length = 0xFF,
              uint16_t delay = 0);
  void stop_rumble();

  void constant(int16_t level, uint16_t length);
  void stop_constant();

  void autocenter(int32_t value = 100);

 protected:
  JoystickForceFeedback();
  virtual void init();

 private:
  int fd_;
  char name_[256];

  bool have_rumble_;
  struct ff_effect rumble_;

  bool have_constant_;
  struct ff_effect constant_;

  bool have_autocenter_;
};

#endif

