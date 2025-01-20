#include "main.h"
#include "liblvgl/core/lv_obj.h"
#include "pros/apix.h"

#include <algorithm>
#include <math.h>
#include <string.h>

#include "ControllerUI.h"
#include "devices.h"
#include "pros/screen.h"
#include "pros/screen.hpp"

using namespace std;

typedef struct
{
  double r; // a fraction between 0 and 1
  double g; // a fraction between 0 and 1
  double b; // a fraction between 0 and 1
} rgb;
typedef struct
{
  double h; // angle in degrees
  double s; // a fraction between 0 and 1
  double v; // a fraction between 0 and 1
} hsv;
static hsv rgb2hsv(rgb in);
static rgb hsv2rgb(hsv in);
hsv rgb2hsv(rgb in)
{
  hsv out;
  double min, max, delta;

  min = in.r < in.g ? in.r : in.g;
  min = min < in.b ? min : in.b;

  max = in.r > in.g ? in.r : in.g;
  max = max > in.b ? max : in.b;

  out.v = max; // v
  delta = max - min;
  if (delta < 0.00001)
  {
    out.s = 0;
    out.h = 0; // undefined, maybe nan?
    return out;
  }
  if (max > 0.0)
  {                        // NOTE: if Max is == 0, this divide would cause a crash
    out.s = (delta / max); // s
  }
  else
  {
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
rgb hsv2rgb(hsv in)
{
  double hh, p, q, t, ff;
  long i;
  rgb out;

  if (in.s <= 0.0)
  { // < is bogus, just shuts up warnings
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

  switch (i)
  {
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

enum CompStatus
{
  DISABLED,
  AUTONOMOUS,
  OPCONTROL,
  DISCONNECTED
};

CompStatus getCompStatus()
{
  if (!competition::is_connected())
    return DISCONNECTED;
  if (competition::is_autonomous())
    return AUTONOMOUS;
  if (competition::is_disabled())
    return DISABLED;
  return OPCONTROL;
}

// string getCompStatusString() {
//   switch (getCompStatus()) {
//   case DISCONNECTED:
//     return "DISCNCTD";
//   case AUTONOMOUS:
//     return "AUTON";
//   case DISABLED:
//     return "DISABLED";
//   case OPCONTROL:
//     return "DRIVER";
//   }
// }

// AUTON STUFF
enum AutonRoutine
{
  RED_LEFT,
  RED_RIGHT,
  BLUE_LEFT,
  BLUE_RIGHT,
  SKILLS,
  NONE
};

AutonRoutine chosen_auton = NONE;

void controller_screen()
{
  // controller has a stupid polling rate of once every 50ms so have to use
  // delays a lot
  delay(50);
  master.clear_line(0);
  delay(50);
  partner.clear_line(0);
  delay(50);

  while (true)
  {

    delay(60);
  }
}

unsigned long createRGB(int r, int g, int b)
{
  return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}
unsigned long createRGBA(int r, int g, int b, int a)
{
  return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) +
         (a & 0xff);
}

void brain_screen()
{
  /*

  LV_IMG_DECLARE(match_field);

  lv_obj_t *img = lv_img_create(lv_scr_act());
  lv_img_set_src(img, &match_field);
  lv_obj_set_pos(img, 120, 0);
  lv_obj_set_size(img, 240, 240);

  lv_obj_t *red_rect = lv_obj_create(lv_scr_act());
  lv_obj_set_size(red_rect, 120, 180);
  lv_obj_set_pos(red_rect, 0, 0);
  lv_obj_set_style_radius(red_rect, 0, 0);
  lv_obj_set_style_pad_all(red_rect, 0, 0);
  lv_obj_set_style_bg_color(red_rect, lv_color_make(235, 25, 25), 0);

  lv_obj_t *blue_rect = lv_obj_create(lv_scr_act());
  lv_obj_set_size(blue_rect, 120, 180);
  lv_obj_set_pos(blue_rect, 360, 0);
  lv_obj_set_style_radius(blue_rect, 0, 0);
  lv_obj_set_style_pad_all(blue_rect, 0, 0);
  lv_obj_set_style_bg_color(blue_rect, lv_color_make(25, 160, 228), 0);

  // skills
  lv_obj_t *skills_rect = lv_obj_create(lv_scr_act());
  lv_obj_set_size(skills_rect, 120, 60);
  lv_obj_set_pos(skills_rect, 360, 180);
  lv_obj_set_style_radius(skills_rect, 0, 0);
  lv_obj_set_style_pad_all(skills_rect, 0, 0);
  lv_obj_set_style_bg_color(skills_rect, lv_color_make(255, 255, 255), 0);

  lv_obj_t *skills_label = lv_label_create(skills_rect);
  lv_label_set_text(skills_label, "SKILLS");
  lv_obj_align(skills_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(skills_label, lv_color_make(0, 0, 0), 0);

  // black box white text for "NONE"
  lv_obj_t *none_rect = lv_obj_create(lv_scr_act());
  lv_obj_set_size(none_rect, 120, 60);
  lv_obj_set_pos(none_rect, 0, 180);
  lv_obj_set_style_radius(none_rect, 0, 0);

  lv_obj_t *none_label = lv_label_create(none_rect);
  lv_label_set_text(none_label, "NONE");
  lv_obj_align(none_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(none_label, lv_color_make(255, 255, 255), 0);

  // lines
  lv_obj_t *line1 = lv_line_create(lv_scr_act());
  static lv_point_t line1_points[] = {{0, 120}, {120, 120}};
  lv_line_set_points(line1, line1_points, 2);
  lv_obj_set_style_line_color(line1, lv_color_make(0, 0, 0), 0);
  lv_obj_set_style_line_width(line1, 2, 0);

  lv_obj_t *line2 = lv_line_create(lv_scr_act());
  static lv_point_t line2_points[] = {{360, 120}, {480, 120}};
  lv_line_set_points(line2, line2_points, 2);
  lv_obj_set_style_line_color(line2, lv_color_make(0, 0, 0), 0);
  lv_obj_set_style_line_width(line2, 2, 0);

  // wait for click
  while (screen::touch_status().touch_status != E_TOUCH_HELD)
  {
    delay(20);
  }

  // top left is red left, bottom left is red right (their perspective facing
  // the field) top right is blue right, bottom right is blue left

  if (screen::touch_status().x < 240)
  {
    if (screen::touch_status().y < 120)
    {
      chosen_auton = RED_LEFT;
    }
    else
    {
      if (screen::touch_status().y < 180 || screen::touch_status().x > 120)
      {
        chosen_auton = RED_RIGHT;
      }
      else
      {
        chosen_auton = NONE;
      }
    }
  }
  else
  {
    if (screen::touch_status().y < 120)
    {
      chosen_auton = BLUE_RIGHT;
    }
    else
    {
      if (screen::touch_status().y < 180 || screen::touch_status().x < 360)
      {
        chosen_auton = BLUE_LEFT;
      }
      else
      {
        chosen_auton = SKILLS;
      }
    }
  }

  // clear screen and display the chosen auton
  screen::erase();
  // switch statement for chosen_auton
  switch (chosen_auton)
  {
  case RED_LEFT:
    screen::print(E_TEXT_LARGE_CENTER, 0, "RED LEFT");
    break;
  case RED_RIGHT:
    screen::print(E_TEXT_LARGE_CENTER, 0, "RED RIGHT");
    break;
  case BLUE_LEFT:
    screen::print(E_TEXT_LARGE_CENTER, 0, "BLUE LEFT");
    break;
  case BLUE_RIGHT:
    screen::print(E_TEXT_LARGE_CENTER, 0, "BLUE RIGHT");
    break;
  case SKILLS:
    screen::print(E_TEXT_LARGE_CENTER, 0, "SKILLS");
    break;
  case NONE:
    screen::print(E_TEXT_LARGE_CENTER, 0, "NO AUTONOMOUS ROUTINE");
    break;
  }

  */

  // wah

  /*
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
  */
}

void initialize()
{
  left_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  right_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);

  chassis.calibrate();

  master.clear();
  delay(50);
  // master.print(0, 0, "Calibrating...");
  // partner.print(0, 0, "Calibrating...");

  // calibration

  // controller task
  // Task controller_task(controller_screen);

  // brain screen task
  // Task brain_task(brain_screen);
}

void disabled() {}

void competition_initialize() {}

void unjammerPlaceholder() {
  while (true) {
    delay(10000);
  }
}
//* AUTON UNJAMMER
Task unjammer = Task(unjammerPlaceholder);
void autonIntakeUnjammer()
{
  double prev_position = chain.get_position();
  while (true)
  {
    prev_position = chain.get_position();
    chain.move_velocity(400);
    delay(150);

    if (abs(prev_position - chain.get_position()) < 1)
    {
      master.rumble("-");
      chain.move_velocity(-600);
      delay(200);
    }

    // if (competition::get_status() != competition_status::COMPETITION_AUTONOMOUS)
    // break;
  }
}

void autonRedRight()
{

  // chassis.setPose({0, 0, 0});

  // chassis.moveToPoint(0, -28, 2000, {.forwards = false, .maxSpeed = 60});
  // chassis.waitUntilDone();


  // lever.set_value(true);
  // chain.move(400);

  // delay(1000);

  // chain.brake();

  chassis.setPose({24, 8, 0});

  hook.set_value(true);
  delay(1000);
  hook.set_value(false);

  // start intake
  roller.move(127);

  chassis.moveToPoint(48, 46, 1000);
  chassis.waitUntilDone();

  delay(200);

  chassis.turnToPoint(0, 48, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.moveToPoint(18, 48, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.waitUntilDone();
  delay(50);

  lever.set_value(true);

  // intake roller
  unjammer = Task(autonIntakeUnjammer);
  delay(3000);
  unjammer.remove();

  chassis.moveToPoint(36, 36, 1000);
  chassis.waitUntilDone();

  left_motors.move(127);
  right_motors.move(-127);

  return;
  /* 3 pointer
  chassis.setPose({31.5, 8, 180}, false);

  roller.move(127);

  chassis.moveToPoint(-48, 48, 2000, {.maxSpeed = 80});
  chassis.waitUntilDone();

  delay(300);

  chassis.turnToPoint(0, 48, 1000, {.forwards = false});
  chassis.moveToPoint(-28, 48, 1000, {.forwards = false, .maxSpeed = 50});

  chassis.waitUntilDone();
  lever.set_value(true);
  delay(100);
  chain.move(127);

  chassis.turnToPoint(0, 24, 2000, {.maxSpeed = 50});

  chassis.waitUntilDone();
  roller.brake();
  */

  // 4 pointer
  chassis.setPose({24, 8, 0}, false);

  chassis.moveToPoint(2, 21, 1500, {.maxSpeed = 80});
  chassis.waitUntilDone();

  // drop the intake
  hook.set_value(true);
  roller.move(127);
  delay(1000);
  hook.set_value(false);

  // chassis.moveToPoint(-12, 12, 1000, { .forwards = false, .maxSpeed = 50 });
  chassis.turnToPoint(22, 46, 1500, {.forwards = false, .maxSpeed = 40});
  chassis.waitUntilDone();
  delay(250);

  // move to right mobile goal
  chassis.moveToPoint(21, 46, 1000, {.forwards = false, .maxSpeed = 70});
  chassis.waitUntilDone();

  delay(200);
  lever.set_value(true);
  delay(200);
  chain.move_velocity(400);

  chassis.turnToPoint(48, 48, 1000);
  chassis.moveToPoint(48, 48, 1000);
  chassis.waitUntilDone();

  delay(500);

  delay(2000);
  return;

  chassis.turnToPoint(0, 36, 2000);
  chassis.moveToPoint(0, 36, 3000, {.earlyExitRange = 5});

  chassis.moveToPoint(-36, 36, 2000);
  chassis.waitUntilDone();

  lever.set_value(false);

  chassis.turnToPoint(-48, 48, 1000);
  chassis.moveToPoint(-48, 48, 1000);
  chassis.waitUntilDone();

  delay(500);

  chassis.turnToPoint(0, 48, 1000, {.forwards = false});
  chassis.moveToPoint(-46, 48, 1000, {.forwards = false});
  chassis.waitUntilDone();

  delay(100);
  lever.set_value(true);
  delay(100);

  chassis.turnToPoint(0, 48, 2000, {.maxSpeed = 50});
  chassis.moveToPoint(-6, 48, 1000, {.maxSpeed = 30});
  chassis.waitUntilDone();

  roller.brake();
}

void autonRedLeft() { 
  chassis.setPose({-24, 8, 0});

  hook.set_value(true);
  delay(1000);
  hook.set_value(false);

  // start intake
  roller.move(127);

  chassis.moveToPoint(-48, 46, 1000);
  chassis.waitUntilDone();

  delay(200);

  chassis.turnToPoint(0, 48, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.moveToPoint(-18, 48, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.waitUntilDone();
  delay(50);

  lever.set_value(true);

  // intake roller
  unjammer = Task(autonIntakeUnjammer);
  delay(3000);
  unjammer.remove();

  chassis.moveToPoint(-36, 36, 1000);
  chassis.waitUntilDone();

  left_motors.move(127);
  right_motors.move(-127);
}

void autonBlueLeft() { chassis.setPose({-24, 8, 180}, false); }

void autonBlueRight() { autonBlueLeft(); }

void autonSkills()
{
  chassis.setPose({18+1, 15+1, 210});

  chassis.moveToPoint(24, 24, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.waitUntilDone();

  delay(300);
  lever.set_value(true);

  unjammer = Task(autonIntakeUnjammer);

  roller.move(127);
  delay(300);

  // testing coords
  // chassis.turnToPoint(0, 24, 1000);
  // chassis.moveToPoint(0, 24, 1000);
  // return;

  // close to ladder ring
  chassis.turnToPoint(24, 48, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(24, 48, 2000);
  chassis.waitUntilDone();
  delay(500);

  chassis.turnToPoint(48, 48, 1000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(48, 48, 2000);
  chassis.waitUntilDone();
  delay(500);

  // right side of the ring triangle
  chassis.turnToPoint(48, 24-24, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(48, 24, 2000);
  chassis.waitUntilDone();
  delay(1000);

  chassis.turnToPoint(60, 24, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(60-4, 24, 2000);
  chassis.waitUntilDone();
  delay(500);

  // go drop the thing in the corner
  chassis.moveToPoint(30, 30, 2000);
  chassis.moveToPoint(60-5, 12+5, 2000, {.forwards = false});
  chassis.waitUntilDone();

  delay(1000);

  unjammer.suspend();
  chain.brake();
  delay(200);
  lever.set_value(false);
  delay(100);

  // ! WAAAA
  // return;


  chassis.moveToPoint(0, 24, 2000);
  chassis.turnToPoint(-24, 24, 2000, {.forwards = false});
  chassis.moveToPoint(-24, 24, 2000, {.forwards = false, .maxSpeed = 60});
  chassis.waitUntilDone();

  // second goal
  delay(300);
  lever.set_value(true);
  delay(300);
  unjammer.resume();

  // ! MIRROR POINT START

  // close to ladder ring
  chassis.turnToPoint(-24, 48, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(-24, 48, 2000);
  chassis.waitUntilDone();
  delay(500);

  // ! extra piece


  // ! end extra piece

  chassis.turnToPoint(-48, 48, 1000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(-48, 48, 2000);
  chassis.waitUntilDone();
  delay(500);

  // right side of the ring triangle
  chassis.turnToPoint(-48, 24-24, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(-48, 24-6, 2000);
  chassis.waitUntilDone();
  delay(1000);

  chassis.turnToPoint(-60, 24, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(-60+4, 24, 2000);
  chassis.waitUntilDone();
  delay(500);

  // go drop the thing in the corner
  chassis.moveToPoint(-30, 30, 2000);
  chassis.moveToPoint(-60+5, 12+5, 2000, {.forwards = false});
  chassis.waitUntilDone();

  delay(1000);

  unjammer.suspend();
  chain.brake();
  delay(200);
  lever.set_value(false);
  delay(100);

  // ! MIRROR POINT END

  chassis.moveToPoint(-48, 72+24, 2000);
  chassis.waitUntilDone();
  chassis.turnToPoint(-24, 72+48, 2000);
  chassis.moveToPoint(-24, 72+44, 2000);
  chassis.waitUntilDone();
  chassis.turnToPoint(0, 0, 2000);
  chassis.turnToPoint(0+24, 72+48, 2000, {.forwards = false});
  chassis.moveToPoint(0-3, 72+48, 2000, {.forwards = false});
  chassis.waitUntilDone();

  delay(200);
  lever.set_value(true);
  delay(200);
  unjammer.resume();

  chassis.turnToPoint(24, 72+24, 2000);
  chassis.moveToPoint(24, 72+24, 2000);
  chassis.waitUntilDone();
  delay(1000);

  unjammer.suspend();
  chain.brake();

  chassis.turnToPoint(0, 72, 2000);
  chassis.waitUntilDone();
  chassis.moveToPoint(0, 72, 2000);
  chassis.waitUntilDone();
  chassis.turnToPoint(-24, 72+24, 2000);
  chassis.waitUntilDone();
  chassis.moveToPoint(-24-4, 72+24+4, 2000);
  chassis.waitUntilDone();

  unjammer.resume();
  delay(1000);



  unjammer.remove();
  
}

void autonomous()
{
  // chassis.calibrate();
  left_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  right_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  switch (chosen_auton)
  {
  case RED_LEFT:
    autonRedLeft();
    break;
  case RED_RIGHT:
    autonRedRight();
    break;
  case BLUE_LEFT:
    autonBlueLeft();
    break;
  case BLUE_RIGHT:
    autonBlueRight();
    break;
  case SKILLS:
    autonSkills();
    break;
  case NONE:
    // THIS ONE NORMALLY
    // autonRedRight();
    // autonRedLeft();

    autonSkills();

    // autonBlueLeft();

    // chassis.moveToPoint(0, 24, 1000);
    // lever.set_value(true);
    // delay(1000);
    // chassis.turnToPoint(24, 0, 1000);
    break;
  }
}

void driverIntakeUnjammer()
{
  double prev_position = chain.get_position();
  int direction = 0;
  int ticker = 0;
  while (true)
  {
    prev_position = chain.get_position();

    if (master.get_digital_new_press(DIGITAL_R1) || master.get_digital_new_press(DIGITAL_R2))
      ticker = 0;
    if (master.get_digital(DIGITAL_R1))
    {
      chain.move_velocity(600);
      direction = 1;
    }
    else if (master.get_digital(DIGITAL_R2))
    {
      chain.move_velocity(-600);
      direction = -1;
    }
    else
    {
      chain.brake();
      direction = 0;
    }

    if (ticker < 5)
      ticker++;

    master.print(0, 0, "%d", (double)ticker);
    delay(50);

    if ((abs(prev_position - chain.get_position()) < 1) && ticker >= 3)
    {
      if (direction != 0)
        master.rumble("-");
      chain.move_velocity(-direction * 600);
      delay(250);
    }

    // if (competition::get_status() != competition_status::COMPETITION_AUTONOMOUS)
    // break;
  }
}

void opcontrol()
{
  left_motors.set_brake_mode(MOTOR_BRAKE_COAST);
  right_motors.set_brake_mode(MOTOR_BRAKE_COAST);

  int turning_speed = 50;

  int chain_speed = 500;

  // bool lever_state = false;
  // bool hook_state = false;


  Task unjammer = Task(driverIntakeUnjammer);

  while (true)
  {
    delay(30);

    if (false)
      continue; // DISABLE THE DRIVE LOOP SO I DONT ACCIDENTALLY DRIVE IT OFF
                // THE TABLE

    // clamp
    if (master.get_digital(E_CONTROLLER_DIGITAL_L1))
      lever.set_value(true);
    else if (master.get_digital(E_CONTROLLER_DIGITAL_L2))
      lever.set_value(false);

    // arm
    if (master.get_digital(E_CONTROLLER_DIGITAL_UP))
      hook.set_value(false);
    else if (master.get_digital(E_CONTROLLER_DIGITAL_DOWN))
      hook.set_value(true);

    // intake roller
    if (master.get_digital(E_CONTROLLER_DIGITAL_X))
    {
      roller.move(127);
    }
    else if (master.get_digital(E_CONTROLLER_DIGITAL_B))
    {
      roller.move(-127);
    }
    else
    {
      roller.move(0);
    }

    if (master.get_digital(E_CONTROLLER_DIGITAL_R1))
    {
      // chain.move_velocity(chain_speed);
      roller.move(127);
    }
    else if (master.get_digital(E_CONTROLLER_DIGITAL_R2))
    {
      // chain.move_velocity(-chain_speed);
      roller.move(-127);
    }
    else
    {
      // chain.move(0);
      roller.move(0);
    }

    double forward = master.get_analog(E_CONTROLLER_ANALOG_LEFT_Y),
           turn = master.get_analog(E_CONTROLLER_ANALOG_RIGHT_X) *
                  (0.01 * turning_speed);

    double left = forward + turn, right = forward - turn;
    double max_val = max(127.0, max(abs(left), abs(right)));
    left /= max_val;
    right /= max_val;

    // if (!master.get_digital(E_CONTROLLER_DIGITAL_R1))
    // {
    left_motors.move(left * 127);
    right_motors.move(right * 127);
    // }
    // else
    // {
    // chassis.curvature(forward, turn);
    // }
  }
}

/*
  rollers.set_value(true);
  chassis.setPose({31, 9, 0}, false);

  chassis.moveToPoint(48, 48, 1000);
  // chassis.waitUntil(36);
  chassis.waitUntilDone();
  intake.move_velocity(200);
  delay(300);
  // delay(350);

  // move to first goal
  chassis.turnToPoint(0, 48, 500, {.forwards = false}, true);
  delay(100);
  intake.brake();
  chassis.waitUntilDone();
  chassis.moveToPoint(24, 48 + 2, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.waitUntilDone();
  delay(150);
  lever.set_value(true);
  intake.move_velocity(200);
  delay(600);
  intake.move_velocity(-100);
  delay(300);
  chassis.turnToPoint(0, 0, 1000);
  chassis.waitUntilDone();

  intake.move_velocity(200);
  chassis.moveToPoint(0, 33, 1500);
  chassis.waitUntilDone();

  intake.move_velocity(200);
  // intake.brake();
  chassis.moveToPoint(-32, 32, 1000);
  chassis.waitUntilDone();

  lever.set_value(false);
  intake.move_velocity(200);
  chassis.turnToPoint(-48, 48, 1000);
  chassis.moveToPoint(-48, 48, 1000, {}, true);
  // chassis.moveToPose(-48, 48, 315, 1500);
  delay(900);
  intake.brake();
  chassis.turnToPoint(-24, 48, 1000, {.forwards = false});
  delay(100);
  chassis.moveToPoint(-18, 48, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.waitUntilDone();
  lever.set_value(true);
  intake.move_velocity(200);
  delay(500);
  chassis.turnToPoint(0, 48, 1000);

  chassis.waitUntilDone();
  delay(300);

  left_motors.set_brake_mode(MOTOR_BRAKE_COAST);
  right_motors.set_brake_mode(MOTOR_BRAKE_COAST);

  intake.move(40);
  chassis.moveToPoint(-6, 52, 800);
  chassis.waitUntilDone();

  lever.set_value(false);

  delay(3000);
  intake.brake();
  */