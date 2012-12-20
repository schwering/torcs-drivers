#include "joystickforcefeedback.h"

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define BITS_PER_LONG         (sizeof(long) * 8)
#define NBITS(x)              ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)                ((x)%BITS_PER_LONG)
#define BIT(x)                (1UL<<OFF(x))
#define LONG(x)               ((x)/BITS_PER_LONG)
#define TEST_BIT(bit, array)  ((array[LONG(bit)] >> OFF(bit)) & 1)


JoystickForceFeedback* JoystickForceFeedback::create()
{
  JoystickForceFeedback* j = new JoystickForceFeedback();
  j->init();
  return j;
}

JoystickForceFeedback::JoystickForceFeedback()
  : fd_(-1),
    have_rumble_(false),
    have_constant_(false),
    have_autocenter_(false)
{
  memset(&rumble_, 0, sizeof(rumble_));
  rumble_.type = FF_RUMBLE;
  rumble_.id   = -1;

  memset(&constant_, 0, sizeof(constant_));
  constant_.type = FF_CONSTANT;
  constant_.id   = -1;
}

JoystickForceFeedback::~JoystickForceFeedback()
{
  stop_rumble();
  stop_constant();
  if (fd_ != -1)
    ::close(fd_);
}

void JoystickForceFeedback::init()
{
  for (int i = 0; i < 128; ++i) {
    char filename[64];
    sprintf(filename, "/dev/input/event%u", i);

    int fd = ::open(filename, O_RDWR);
    if (fd == -1) {
      continue;
    }

    fprintf(stderr, "Event file %s\n", filename);

    char name[256];
    sprintf(name, "Unknown");
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
      close(fd);
      continue;
    }

    fprintf(stderr, "Device name %s\n", name);

    long features[NBITS(EV_MAX)];
    memset(features, 0, sizeof(features));
    if (ioctl(fd, EVIOCGBIT(0, EV_MAX), features) < 0) {
      close(fd);
      fprintf(stderr, "Cannot get feedback feature vector of device %s\n", name);
      continue;
    }

    if (!TEST_BIT(EV_FF, features)) {
      close(fd);
      fprintf(stderr, "Device %s does not support force-feedback\n", name);
      continue;
    }

    long ff_features[NBITS(FF_MAX)];
    memset(ff_features, 0, sizeof(ff_features));
    if (ioctl(fd, EVIOCGBIT(EV_FF, FF_MAX), ff_features) < 0) {
      close(fd);
      fprintf(stderr, "Cannot get device %s force feedback feature vector\n", name);
      continue;
    }

    long no_ff_features[NBITS(FF_MAX)];
    memset(no_ff_features, 0, sizeof(no_ff_features));
    if (memcmp(ff_features, no_ff_features, sizeof(no_ff_features)) == 0) {
      close(fd);
      fprintf(stderr, "Device %s has no force feedback features\n", name);
      continue;
    }

    if (TEST_BIT(FF_CONSTANT, ff_features))   printf("Constant\n");
    if (TEST_BIT(FF_PERIODIC, ff_features))   printf("Periodic\n");
    if (TEST_BIT(FF_SPRING, ff_features))     printf("Spring\n");
    if (TEST_BIT(FF_FRICTION, ff_features))   printf("Friction\n");
    if (TEST_BIT(FF_RUMBLE, ff_features))     printf("Rumble\n");
    if (TEST_BIT(FF_AUTOCENTER, ff_features)) printf("Autocenter\n");

    have_rumble_ = TEST_BIT(FF_RUMBLE, ff_features) != 0;
    have_constant_ = TEST_BIT(FF_CONSTANT, ff_features) != 0;
    have_autocenter_ = TEST_BIT(FF_AUTOCENTER, ff_features) != 0;
    if (have_rumble_ || have_constant_) {
      fd_ = fd;
      strcpy(name_, name);
      break;
    }
  }

  if (fd_ == -1) {
    throw Exception("Cannot open joystick.");
  }
}

void
JoystickForceFeedback::rumble(uint16_t strong_magnitude,
                              uint16_t weak_magnitude,
                              Direction direction,
                              uint16_t length,
                              uint16_t delay)
{
  if (!have_rumble_) {
    throw Exception("Rumble is not supported.");
  }

  rumble_.u.rumble.strong_magnitude = strong_magnitude;
  rumble_.u.rumble.weak_magnitude   = weak_magnitude;
  rumble_.direction = direction;
  rumble_.replay.length = length;
  rumble_.replay.delay = delay;

  if (ioctl(fd_, EVIOCSFF, &rumble_) < 0) {
    throw Exception("Failed to upload rumble effect");
  }

  struct input_event play;
  play.type = EV_FF;
  play.code = rumble_.id;
  play.value = 1;

  if (write(fd_, &play, sizeof(play)) < 0) {
    throw Exception("Failed to start rumble effect");
  }
}

void JoystickForceFeedback::stop_rumble()
{
  if (fd_ != -1) {
    if (rumble_.id != -1) {
      if (ioctl(fd_, EVIOCRMFF, rumble_.id) < 0) {
        throw Exception("Failed to stop rumble effect");
      }
      rumble_.id = -1;
    }
  }
}

void
JoystickForceFeedback::constant(int16_t level,
                                uint16_t length)
{
  if (!have_constant_) {
    throw Exception("Constant is not supported.");
  }

  constant_.type = FF_CONSTANT;
  constant_.id = -1;
  constant_.u.constant.level = 0x7000; /* Strength : 25 % */
  constant_.direction = 0;        /* 135 degrees */
  constant_.u.constant.envelope.attack_length = 0x100;
  constant_.u.constant.envelope.attack_level = 0;
  constant_.u.constant.envelope.fade_length = 0x100;
  constant_.u.constant.envelope.fade_level = 0;
  constant_.trigger.button = 0;
  constant_.trigger.interval = 0;
  constant_.replay.length = 20000;  /* 20 seconds */
  constant_.replay.delay = 0;

  fprintf(stderr, "Uploading constant effect.\n");
  if (ioctl(fd_, EVIOCSFF, &constant_) < 0) {
    fprintf(stderr, "Upload error %d '%s'\n", errno, strerror(errno));
    throw Exception("Failed to upload constant effect");
  }

  struct input_event play;
  play.type = EV_FF;
  play.code = constant_.id;
  play.value = 1;

  if (write(fd_, &play, sizeof(play)) < 0) {
    throw Exception("Failed to start constant effect");
  }
}

void JoystickForceFeedback::stop_constant()
{
  if (fd_ != -1) {
    if (constant_.id != -1) {
      if (ioctl(fd_, EVIOCRMFF, constant_.id) < 0) {
        throw Exception("Failed to stop constant effect");
      }
      constant_.id = -1;
    }
  }
}

void JoystickForceFeedback::autocenter(int32_t value)
{
  if (!have_autocenter_) {
    throw Exception("Constant is not supported.");
  }

  struct input_event center;
  center.type = EV_FF;
  center.code = FF_AUTOCENTER;
  center.value = 0xFFFFUL * value / 100;

  if (write(fd_, &center, sizeof(center)) < 0) {
    throw Exception("Failed to perform autocenter effect");
  }
  printf("Sent autocenter.\n");
}

