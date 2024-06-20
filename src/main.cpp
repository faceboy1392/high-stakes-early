#include "main.h"
// #include "pros/apix.h"

#include <algorithm>
#include <math.h>
#include <string.h>

#include "ControllerUI.h"
#include "devices.h"
#include "pros/screen.hpp"

using namespace std;

typedef struct {
  double r; // a fraction between 0 and 1
  double g; // a fraction between 0 and 1
  double b; // a fraction between 0 and 1
} rgb;

typedef struct {
  double h; // angle in degrees
  double s; // a fraction between 0 and 1
  double v; // a fraction between 0 and 1
} hsv;

static hsv rgb2hsv(rgb in);
static rgb hsv2rgb(hsv in);

hsv rgb2hsv(rgb in) {
  hsv out;
  double min, max, delta;

  min = in.r < in.g ? in.r : in.g;
  min = min < in.b ? min : in.b;

  max = in.r > in.g ? in.r : in.g;
  max = max > in.b ? max : in.b;

  out.v = max; // v
  delta = max - min;
  if (delta < 0.00001) {
    out.s = 0;
    out.h = 0; // undefined, maybe nan?
    return out;
  }
  if (max > 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
    out.s = (delta / max); // s
  } else {
    // if max is 0, then r = g = b = 0
    // s = 0, h is undefined
    out.s = 0.0;
    out.h = NAN; // its now undefined
    return out;
  }
  if (in.r >= max)                 // > is bogus, just keeps compilor happy
    out.h = (in.g - in.b) / delta; // between yellow & magenta
  else if (in.g >= max)
    out.h = 2.0 + (in.b - in.r) / delta; // between cyan & yellow
  else
    out.h = 4.0 + (in.r - in.g) / delta; // between magenta & cyan

  out.h *= 60.0; // degrees

  if (out.h < 0.0)
    out.h += 360.0;

  return out;
}

rgb hsv2rgb(hsv in) {
  double hh, p, q, t, ff;
  long i;
  rgb out;

  if (in.s <= 0.0) { // < is bogus, just shuts up warnings
    out.r = in.v;
    out.g = in.v;
    out.b = in.v;
    return out;
  }
  hh = in.h;
  if (hh >= 360.0)
    hh = 0.0;
  hh /= 60.0;
  i = (long)hh;
  ff = hh - i;
  p = in.v * (1.0 - in.s);
  q = in.v * (1.0 - (in.s * ff));
  t = in.v * (1.0 - (in.s * (1.0 - ff)));

  switch (i) {
  case 0:
    out.r = in.v;
    out.g = t;
    out.b = p;
    break;
  case 1:
    out.r = q;
    out.g = in.v;
    out.b = p;
    break;
  case 2:
    out.r = p;
    out.g = in.v;
    out.b = t;
    break;

  case 3:
    out.r = p;
    out.g = q;
    out.b = in.v;
    break;
  case 4:
    out.r = t;
    out.g = p;
    out.b = in.v;
    break;
  case 5:
  default:
    out.r = in.v;
    out.g = p;
    out.b = q;
    break;
  }
  return out;
}

enum CompStatus { DISABLED, AUTONOMOUS, OPCONTROL, DISCONNECTED };

CompStatus getCompStatus() {
  if (!competition::is_connected())
    return DISCONNECTED;
  if (competition::is_autonomous())
    return AUTONOMOUS;
  if (competition::is_disabled())
    return DISABLED;
  return OPCONTROL;
}

string getCompStatusString() {
  switch (getCompStatus()) {
  case DISCONNECTED:
    return "DISCNCTD";
  case AUTONOMOUS:
    return "AUTON";
  case DISABLED:
    return "DISABLED";
  case OPCONTROL:
    return "DRIVER";
  }
}

void controller_screen() {
  // controller has a stupid polling rate of once every 50ms so have to use
  // delays a lot
  delay(50);
  master.clear_line(0);
  delay(50);
  partner.clear_line(0);
  delay(50);

  while (true) {

    delay(60);
  }
}

unsigned long createRGB(int r, int g, int b)
{   
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}
unsigned long createRGBA(int r, int g, int b, int a) {
  return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) +
         (a & 0xff);
}

void brain_screen() {
  screen::erase();
  double hue = 0;
  while (true) {
    delay(20);
    // convert hue to rgb with full saturation and value
    rgb color = hsv2rgb({hue, 1, 1});
    screen::set_pen(
        createRGB(color.r * 255, color.g * 255, color.b * 255));
    if (hue < 360)
      hue += 5;
    else
      hue = 0;
    screen::fill_rect(0, 0, 480, 240);
    // screen::set_pen(0xFF0000);
  }
}

void initialize() {
  master.clear();
  delay(50);
  master.print(0, 0, "Calibrating...");
  partner.print(0, 0, "Calibrating...");

  // stdout
  FILE *out = fopen("sout", "w");

  // calibration
  chassis.calibrate();
  fputs("Calibrated chassis", out);

  // settings file
  fputs("Opening settings.txt...", out);
  FILE *fp = fopen("/settings.txt", "r");
  if (fp == NULL) {
    fputs("settings.txt not found, creating...", out);
    fp = fopen("/settings.txt", "w");
    fprintf(fp, "");
    fclose(fp);
  } else
    fputs("settings.txt found", out);

  // controller task
  Task controller_task(controller_screen);
  fputs("Started controller screen task", out);

  // brain screen task
  Task brain_task(brain_screen);
  fputs("Started brain screen task", out);
}

void disabled() {}

void competition_initialize() {}

void autonomous() {}

void opcontrol() {
  int turning_speed = 50;

  while (true) {
    delay(20);

    if (false)
      continue; // DISABLE THE DRIVE LOOP SO I DONT ACCIDENTALLY DRIVE IT OFF
                // THE TABLE

    if (master.get_digital_new_press(E_CONTROLLER_DIGITAL_UP))
      turning_speed += turning_speed >= 100 ? 0 : 10;
    if (master.get_digital_new_press(E_CONTROLLER_DIGITAL_DOWN))
      turning_speed -= turning_speed <= 0 ? 0 : 10;

    double forward = master.get_analog(E_CONTROLLER_ANALOG_LEFT_Y),
           turn = master.get_analog(E_CONTROLLER_ANALOG_RIGHT_X) *
                  (0.01 * turning_speed);

    double left = forward + turn, right = forward - turn;
    double max_val = max(127.0, max(abs(left), abs(right)));
    left /= max_val;
    right /= max_val;

    if (!master.get_digital(E_CONTROLLER_DIGITAL_R1)) {
      left_motors.move(left * 127);
      right_motors.move(right * 127);
    } else {
      chassis.curvature(forward, turn);
    }
  }
}