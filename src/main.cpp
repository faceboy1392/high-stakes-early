#include "main.h"
#include "liblvgl/core/lv_obj.h"
// #include "pros/apix.h"

#include <algorithm>
#include <math.h>
#include <string.h>

#include "ControllerUI.h"
#include "devices.h"
#include "pros/screen.h"
#include "pros/screen.hpp"

#include "PID.h"

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
  DRIVE_FORWARD,
  NONE
};

AutonRoutine chosen_auton = DRIVE_FORWARD;
bool auton_selector_active = false;

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

void printToController(string line1, string line2, string line3)
{
  master.print(0, 0, line1.c_str());
  delay(51);
  master.print(1, 0, line2.c_str());
  delay(51);
  master.print(2, 0, line3.c_str());
  delay(51);
}

void controllerAutonSelector()
{
  auton_selector_active = true;

  int cursor_x = 1, cursor_y = 2;
  bool selected = false;

  string auton_options[6] = {"Red.L", "Blu.R", "Red.R", "Blu.L", "Skills", "EXIT"};
  AutonRoutine auton_options_enums[6] = {RED_LEFT, BLUE_RIGHT, RED_RIGHT, BLUE_LEFT, SKILLS, NONE};

  // ", -, and [] are the same char width so they can be swapped without messing things up

  int ticker = 0;

  while (true)
  {
    ticker++;

    if (master.get_digital_new_press(DIGITAL_A) || master.get_digital_new_press(DIGITAL_Y))
    {
      if (selected == false)
        selected = true;
      else
        selected = false;
    }
    if (master.get_digital_new_press(DIGITAL_UP) && !selected)
    {
      cursor_y--;
      if (cursor_y < 0)
        cursor_y = 0;
    }
    if (master.get_digital_new_press(DIGITAL_DOWN) && !selected)
    {
      cursor_y++;
      if (cursor_y > 2)
        cursor_y = 2;
    }
    if (master.get_digital_new_press(DIGITAL_LEFT) && !selected)
    {
      cursor_x--;
      if (cursor_x < 0)
        cursor_x = 0;
    }
    if (master.get_digital_new_press(DIGITAL_RIGHT) && !selected)
    {
      cursor_x++;
      if (cursor_x > 1)
        cursor_x = 1;
    }

    string lines[3] = {"", "", ""};
    // add each auton option to the strings
    for (int i = 0; i < 3; i++)
    {

      if (cursor_x == 0 && cursor_y == i)
      {
        if (selected)
          lines[i] += "[";
        else
          lines[i] += "\"";
      }
      else
      {
        lines[i] += "-";
      }

      lines[i] += auton_options[i * 2];

      if (cursor_x == 0 && cursor_y == i)
      {
        if (selected)
          lines[i] += "]";
        else
          lines[i] += "-";
      }
      else if (cursor_x == 1 && cursor_y == i)
      {
        if (selected)
          lines[i] += "[";
        else
          lines[i] += "-";
      }
      else
      {
        lines[i] += "-";
      }

      lines[i] += auton_options[i * 2 + 1];

      if (cursor_x == 1 && cursor_y == i)
      {
        if (selected)
          lines[i] += "]";
        else
          lines[i] += "\"";
      }
      else
      {
        lines[i] += "-";
      }
    }

    if (ticker > 15)
    {
      ticker = 0;
      printToController(lines[0], lines[1], lines[2]);
      if (selected)
        chosen_auton = auton_options_enums[cursor_y * 2 + cursor_x];
      else
        chosen_auton = DRIVE_FORWARD;
    }

    delay(11);

    if (!auton_selector_active || (chosen_auton == NONE && selected))
    {
      auton_selector_active = false;
      master.clear();
      break;
    }
  }
}

int ladybrown_state;
double lb_offset;

void initialize()
{
  left_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  right_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);

  chassis.calibrate();

  master.clear();

  double lb_sensor_pos = ladybrown_sensor.get_position() / 100.0;

  delay(80);
  // print the angle
  master.print(0, 0, "Angle: ");
  delay(80);
  master.print(1, 0, to_string(lb_sensor_pos).c_str());
  ladybrown_state = lb_sensor_pos > 10 ? 1 : 0;
  lb_offset = ladybrown_state == 1 ? lb_sensor_pos * 5 : 0;

  delay(50);
  // master.print(0, 0, "Calibrating...");
  // partner.print(0, 0, "Calibrating...");

  // calibration

  // controller task
  Task controller_task(controllerAutonSelector);

  ring_detector.set_led_pwm(100);
  ring_detector.disable_gesture();

  // brain screen task
  // Task brain_task(brain_screen);
}

void disabled() {}

void competition_initialize() {}

void unjammerPlaceholder()
{
  while (true)
  {
    delay(1000000);
  }
}

enum RingColor
{
  RED_RING,
  BLUE_RING,
  NO_COLOR
};

RingColor last_detected_ring_color = NO_COLOR;
RingColor eject_color = NO_COLOR;
bool eject_toggle = true;
int previous_ejection_motor_pos = 0;

int ring_ticker = 0;

void detectRingColor()
{
  // blue 210
  // red 20 ish

  ring_ticker++;

  // master.print(0, 0, to_string(ring_detector.get_hue()).c_str());

  if (/*tick % 5 == 0 &&*/ ring_distance.get_distance() < 40)
  {
    int hue = ring_detector.get_hue();
    // if (hue > 180 && hue < 280)
    if (hue > 100)
    {
      last_detected_ring_color = BLUE_RING;
      ring_ticker = 0;
      // master.print(0, 0, "BLUE");
      // delay(51);
      // master.rumble(".-");
      screen::set_pen(Color::blue);
      screen::draw_rect(0, 0, 480, 240);
    }
    // else if (hue > 0 && hue < 50)
    else if (hue < 50)
    {
      last_detected_ring_color = RED_RING;
      ring_ticker = 0;
      // master.print(0, 0, "RED");
      // delay(51);
      // master.rumble(".-");
      screen::set_pen(Color::red);
      screen::draw_rect(0, 0, 480, 240);
    }
  }

  // make sure to update this if i update the delay timer
  if (ring_ticker == 10)
  {
    last_detected_ring_color = NO_COLOR;
    // master.clear();
  }
}

void universalIntakeController() {
  int unjam_ticker = 0;
  int eject_ticker = 0;
  while (true) {
    bool jammed = false;
    bool eject = false;


    if (jammed) {
      unjam_ticker = 0;
    }

    if (eject) {
      eject_ticker = 0;
      master.rumble("-");
      chain.move_velocity(-600);
      delay(200);
    }
    delay(10);
  }
}

Mutex unjammer_mutex;



//* AUTON UNJAMMER
Task unjammer = Task(unjammerPlaceholder);
void autonIntakeUnjammer()
{
  if (chosen_auton == SKILLS)
    unjammer_mutex.take(59 * 1000);
  else
    unjammer_mutex.take(14.5 * 1000);

  double prev_position = chain.get_position();
  eject_color = BLUE_RING;
  eject_toggle = true;
  int eject_ticker = 0;
  int ticker = 0;
  while (true)
  {
    detectRingColor();
    bool distance_triggered = chain_detector.get_distance() < 40;
    bool color_matched = last_detected_ring_color == eject_color;
    bool eject_ticker_ok = eject_ticker > 4;

    //* GAP FILLING
    double chain_diff = abs(previous_ejection_motor_pos - chain.get_position());
    bool gap_filler = false;

    if (chain_diff / 360.0 >= 4.0)
    {
      gap_filler = true;
    }

    if ((distance_triggered || gap_filler) && eject_ticker_ok)
    {
      previous_ejection_motor_pos = chain.get_position();
      // master.rumble(".");
    }

    bool eject = eject_toggle &&
                 eject_ticker_ok &&
                 (distance_triggered || gap_filler) &&
                 color_matched &&
                 eject_color != NO_COLOR;

    if (eject)
      eject_ticker = 0;
    else if (eject_ticker < 10)
      eject_ticker++;

    prev_position = chain.get_position();
    chain.move_velocity(600);
    delay(10);

    if (eject)
    {
      // disable_gap_filler = true;
      chain.set_brake_mode(MOTOR_BRAKE_BRAKE);
      chain.brake();
      master.rumble(".");
      delay(100);
      chain.set_brake_mode(MOTOR_BRAKE_COAST);
      ticker = 0;
    }

    if (ticker < 40)
      ticker++;

    if (abs(prev_position - chain.get_position()) < 1 && ticker >= 15)
    {
      master.rumble("-");
      chain.move_velocity(-600);
      delay(200);
    }

    // if (competition::get_status() != competition_status::COMPETITION_AUTONOMOUS)
    // {
    //   break;
    // }

    if (!unjammer_mutex.take(0))
    {
      pros::Task::current().remove();
      break;
    }
  }
}

void wait()
{
  chassis.waitUntilDone();
}

//! ---------------------------------------
//! CLOSE AUTONOMOUS ROUTINE
//! ---------------------------------------
/**
 * starts LEFT side by default
 */
void closeAutonRoutine(bool mirror = false)
{
  int r = mirror ? -1 : 1;

  chassis.setPose({r * 24, 7.5+2, 0}, false);

  // //top of the middle stack
  // chassis.moveToPose(r*8, 24, -90, 1000);
  // wait();

  // hook.set_value(true);
  // roller.move(127);
  // delay(500);
  // hook.set_value(false);

  // chassis.turnToPoint(r*24, 48, 1000, {.forwards = false});
  // wait();
  // chassis.moveToPoint(r*22, 44, 1000, {.forwards = false, .maxSpeed = 60});
  // wait();
  // delay(200);

  // lever.set_value(true);
  // delay(100);

  roller.move(127);
  hook.set_value(true);
  delay(500);

  //* stack adjacent to the goal
  chassis.moveToPoint(r * (48 + 2), 48 + 2, 1000);
  wait();

  hook.set_value(false);
  delay(400);

  chassis.turnToPoint(r * 24, 48 + 1, 1000, {.forwards = false});
  wait();
  chassis.moveToPoint(r * (24 - 4), 48 + 1, 1000, {.forwards = false, .maxSpeed = 45});
  wait();

  //* grab goal
  // delay(100);
  lever.set_value(true);
  Task intake_unjammer = Task(autonIntakeUnjammer);

  delay(200);

  //* move to the centered cluster
  chassis.turnToPoint(r * 48, 72, 1000);
  wait();
  chassis.moveToPoint(r * (48 - 9), 72 - 10, 1000);
  wait();
  // hook.set_value(true);
  delay(200);
  //* second ring
  chassis.turnToPoint(r * 48, 72 - 10, 1000);
  wait();
  chassis.moveToPoint(r * 48, 72 - 10, 1000);
  wait();
  delay(200);
  //* move back
  chassis.moveToPoint(r * 48, 48, 2000, {.forwards = false, .maxSpeed = 70});
  wait();
  // hook.set_value(false);
  delay(200);
  if (r == 1)
  {
    chassis.moveToPose(r * (18), (24), 250, 3000);
    wait();
    hook.set_value(true);
    delay(400);
    chassis.turnToPoint(r * -24, 48, 500);
    wait();
    delay(200);
    hook.set_value(false);
    chassis.moveToPoint(r * 12, 36, 1000);
    wait();
    delay(800);
    chassis.moveToPoint(r * 8, 40, 500);
  }
  else
  {
  }
}

//! ---------------------------------------
//! FAR AUTONOMOUS ROUTINE
//! ---------------------------------------
/**
 * starts LEFT side by default
 */
void farAutonRoutine(bool mirror = false)
{
  int r = mirror ? -1 : 1;
}

//! ---------------------------------------
//! RED LEFT AUTONOMOUS
//! ---------------------------------------
void autonRedLeft() {}

//! ---------------------------------------
//! RED RIGHT AUTONOMOUS
//! ---------------------------------------
void autonRedRight() {}

//! ---------------------------------------
//! BLUE LEFT AUTONOMOUS
//! ---------------------------------------
void autonBlueLeft() {}

//! ---------------------------------------
//! BLUE RIGHT AUTONOMOUS
//! ---------------------------------------
void autonBlueRight() {}

//! ---------------------------------------
//! SKILLS AUTONOMOUS
//! ---------------------------------------
void autonSkills()
{
  chassis.setPose({18 + 1, 15 + 3, 210});

  chassis.moveToPoint(24, 24, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.waitUntilDone();

  delay(300);
  lever.set_value(true);

  unjammer.remove();
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
  chassis.turnToPoint(48, 24 - 24, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(48, 24, 2000);
  chassis.waitUntilDone();
  delay(1000);

  chassis.turnToPoint(60, 28, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(60 - 4, 24, 2000);
  chassis.waitUntilDone();
  delay(1500);

  // go drop the thing in the corner
  chassis.moveToPoint(30, 30, 2000, {.maxSpeed = 40});
  chassis.moveToPoint(60 - 5, 12 + 5, 2000, {.forwards = false});
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
  chassis.turnToPoint(-48, 24 - 24, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(-48, 24 - 6, 2000);
  chassis.waitUntilDone();
  delay(1000);

  chassis.turnToPoint(-60, 24, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(-60 + 4, 24, 2000);
  chassis.waitUntilDone();
  delay(1500);

  // go drop the thing in the corner
  chassis.moveToPoint(-30, 30, 2000);
  chassis.moveToPoint(-60 - 5, 12 - 5, 2000, {.forwards = false});
  chassis.waitUntilDone();

  delay(1000);

  unjammer.suspend();
  chain.brake();
  delay(200);
  lever.set_value(false);
  delay(100);

  // ! MIRROR POINT END

  // chassis.moveToPoint(-38, 36, 1000);
  // chassis.turnToPoint(0 - 3, 72 + 48, 2000, {.forwards = false});
  // chassis.moveToPoint(0 - 3, 72 + 48, 2000, {.forwards = false});
  // chassis.waitUntilDone();
  chassis.moveToPoint(-48, 72, 3000);

  chassis.moveToPoint(0, 72 + 60, 3000);
  chassis.turnToPoint(-24, 72 + 60, 2000, {.forwards = false});
  chassis.moveToPoint(-24, 72 + 60, 2000, {.forwards = false, .maxSpeed = 40});

  chassis.waitUntilDone();

  delay(200);
  lever.set_value(true);
  delay(200);

  chassis.turnToPoint(-72, 72 + 72, 1000);

  chassis.turnToPoint(-72, 72 + 72, 2000, {.forwards = false});
  chassis.moveToPoint(-72 + 5, 72 + 72 - 5, 4000, {.forwards = false});
  chassis.waitUntilDone();
  delay(200);

  lever.set_value(false);

  // chassis.moveToPoint(-48, 72 + 24, 2000);
  // chassis.waitUntilDone();
  // chassis.turnToPoint(-24, 72 + 48, 2000);
  // chassis.moveToPoint(-24, 72 + 44, 2000);
  // chassis.waitUntilDone();
  // chassis.turnToPoint(0, 0, 2000);
  // chassis.turnToPoint(0 + 24, 72 + 48, 2000, {.forwards = false});
  // chassis.moveToPoint(0 - 3, 72 + 48, 2000, {.forwards = false});
  // chassis.waitUntilDone();

  // delay(200);
  // lever.set_value(true);
  // delay(200);
  // unjammer.resume();

  // chassis.turnToPoint(24, 72 + 24, 2000);
  // chassis.moveToPoint(24, 72 + 24, 2000);
  // chassis.waitUntilDone();
  // delay(1000);

  // unjammer.suspend();
  // chain.brake();

  // chassis.turnToPoint(0, 72, 2000);
  // chassis.waitUntilDone();
  // chassis.moveToPoint(0, 72, 2000);
  // chassis.waitUntilDone();
  // chassis.turnToPoint(-24, 72 + 24, 2000);
  // chassis.waitUntilDone();
  // chassis.moveToPoint(-24 - 4, 72 + 24 + 4, 2000);
  // chassis.waitUntilDone();

  // unjammer.resume();
  // delay(1000);

  // unjammer.remove();
  unjammer.remove();
}

//! ---------------------------------------
//! DRIVE FORWARD AUTONOMOUS
//! ---------------------------------------
void autonDriveForward()
{
  chassis.tank(30, 30);
  delay(800);
  chassis.tank(0, 0);
}

void autonomous()
{
  // chassis.calibrate();
  left_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  right_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  auton_selector_active = false;
  // autonDriveForward();

  // RIGHT I THINK
  // closeAutonRoutine(false);
  // LEFT I THINK
  closeAutonRoutine(true);
  return;
  chosen_auton = RED_LEFT;

  switch (chosen_auton)
  {
  case RED_LEFT:
    // autonRedLeft();
    // farAutonRoutine(true);
    break;
  case RED_RIGHT:
    closeAutonRoutine();
    break;
  case BLUE_LEFT:
    closeAutonRoutine(true);
    break;
  case BLUE_RIGHT:
    farAutonRoutine();
    break;
  case SKILLS:
    autonSkills();
    break;
  case DRIVE_FORWARD:
    autonDriveForward();
    break;
  case NONE:
    // THIS ONE NORMALLY
    // autonRedRight();
    // autonRedLeft();

    // autonSkills();

    // autonBlueLeft();

    // doubleSidedAuton(-1, false);

    // chassis.moveToPoint(0, 24, 1000);
    // lever.set_value(true);
    // delay(1000);
    // chassis.turnToPoint(24, 0, 1000);
    break;
  }
}

void driverIntakeUnjammer()
{
  while (unjammer_mutex.take(0))
  {
    delay(50);
  }

  double prev_position = chain.get_position();
  int direction = 0;
  int ticker = 0;
  int eject_ticker = 0;
  while (true)
  {
    // //* EJECTION CODE
    // double chain_diff = abs(chain.get_position() - previous_ejection_motor_pos) / 360.0;
    // // master.print(0, 0, to_string(chain_diff).c_str());
    // if (previous_ejection_motor_pos == 0) chain_diff = 1000000;
    // if (chain_diff > 1.0 && chain_diff < 2.0)
    //   disable_gap_filler = false;
    // double chain_diff_mod = fmod(chain_diff, 4.0);
    // bool gap_filler = (chain_diff_mod >= 0.9 || chain_diff_mod <= 0.1) && !disable_gap_filler;

    // if (gap_filler || (chain_detector.get_distance() < 40)) {
    //   master.rumble(".");
    //   disable_gap_filler = true;
    // }
    // if (gap_filler) master.rumble(".");
    // if (chain_detector.get_distance() < 40) master.rumble("-");

    bool distance_triggered = chain_detector.get_distance() < 40;
    bool color_matched = last_detected_ring_color == eject_color;
    bool eject_ticker_ok = eject_ticker > 4;

    //* GAP FILLING
    double chain_diff = abs(previous_ejection_motor_pos - chain.get_position());
    bool gap_filler = false;

    if (chain_diff / 360.0 >= 4.0)
    {
      gap_filler = true;
    }

    if ((distance_triggered || gap_filler) && eject_ticker_ok)
    {
      previous_ejection_motor_pos = chain.get_position();
      // master.rumble(".");
    }

    bool eject = eject_toggle &&
                 eject_ticker_ok &&
                 (distance_triggered || gap_filler) &&
                 color_matched &&
                 eject_color != NO_COLOR;

    if (eject)
      eject_ticker = 0;
    else if (eject_ticker < 10)
      eject_ticker++;

    // bool eject = eject_toggle &&
    //  (chain_detector.get_distance() < 40 || gap_filler) &&
    //  last_detected_ring_color == eject_color &&
    //  eject_color != NO_COLOR;

    // if (chain_detector.get_distance() < 50)
    //   master.rumble("..");

    //* some normal anti jam stuff
    prev_position = chain.get_position();

    if (
        master.get_digital_new_press(DIGITAL_R1) ||
        master.get_digital_new_press(DIGITAL_R2) ||
        eject)
      ticker = 0;
    if (master.get_digital(DIGITAL_R1))
    {
      if (ladybrown_state == 1)
      {
        chain.move_velocity(400);
      }
      else
      {
        chain.move_velocity(600);
      }
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

    if (eject)
    {
      // disable_gap_filler = true;
      chain.set_brake_mode(MOTOR_BRAKE_BRAKE);
      chain.brake();
      master.rumble(".");
      delay(100);
      chain.set_brake_mode(MOTOR_BRAKE_COAST);
    }

    if (ticker < 40)
      ticker++;

    delay(10);

    //* JAM CONDITION
    if ((abs(prev_position - chain.get_position()) < 1) && ticker >= 15)
    {
      if (direction != 0)
        master.rumble("-");
      chain.move_velocity(-direction * 600);
      delay(300);
    }

    // if (competition::get_status() != competition_status::COMPETITION_AUTONOMOUS)
    // break;
  }
}

void opcontrol()
{
  delay(51);
  master.clear();

  auton_selector_active = false;

  eject_toggle = false;

  left_motors.set_brake_mode(MOTOR_BRAKE_COAST);
  right_motors.set_brake_mode(MOTOR_BRAKE_COAST);

  int turning_speed = 60;

  // int chain_speed = 500;

  // bool lever_state = false;
  // bool hook_state = false;

  Task unjammer = Task(driverIntakeUnjammer);

  bool ladybrown_oncer = false;
  int ladybrown_ticker = 0;

  // set color based on which auton is selected
  if (chosen_auton == RED_LEFT || chosen_auton == RED_RIGHT)
  {
    eject_color = RED_RING;
  }
  else if (chosen_auton == BLUE_LEFT || chosen_auton == BLUE_RIGHT)
  {
    eject_color = BLUE_RING;
  }

  ring_detector.set_led_pwm(50);

  ulong tick = 0;

  while (true)
  {
    delay(20);
    tick++;

    if (false)
      continue; // DISABLE THE DRIVE LOOP SO I DONT ACCIDENTALLY DRIVE IT OFF
                // THE TABLE

    detectRingColor();

    // clamp
    if (master.get_digital(DIGITAL_L1))
      lever.set_value(true);
    else if (master.get_digital(DIGITAL_L2))
      lever.set_value(false);

    // face buttons are used for auton selection
    if (!auton_selector_active)
    {
      // arm
      if (master.get_digital(DIGITAL_UP))
        hook.set_value(false);
      else if (master.get_digital(DIGITAL_DOWN))
        hook.set_value(true);

      //* ladybrown
      // left/right stows it, b readies it, y and a extend it halfway, and x goes all the way
      // if (master.get_digital(DIGITAL_LEFT) ||
      // master.get_digital(DIGITAL_RIGHT))
      // ladybrown_state = 3;
      // else
      if (master.get_digital(DIGITAL_B))
        ladybrown_state = 0;
      else if (master.get_digital(DIGITAL_Y) ||
               master.get_digital(DIGITAL_A))
        ladybrown_state = 1;
      else if (master.get_digital(DIGITAL_X))
        ladybrown_state = 2;

      //* set eject color based on button left
      if (master.get_digital_new_press(DIGITAL_LEFT))
      {
        if (eject_color == RED_RING)
        {

          master.print(1, 0, "EJECT BLUE");
          eject_color = BLUE_RING;
        }
        else if (eject_color == BLUE_RING)
        {

          master.print(1, 0, "EJECT RED");
          eject_color = RED_RING;
        }
        else
        {
          master.print(1, 0, "EJECT RED");
          eject_color = RED_RING;
        }
      }
      if (master.get_digital_new_press(DIGITAL_RIGHT))
      {
        eject_color == NO_COLOR;
      }
    }

    //* ladybrown stuff
    ladybrown_ticker++;
    if (ladybrown_ticker > 50 && ladybrown_state == 1)
    {
      master.rumble(".");
      ladybrown_ticker = 0;
    }

    if (ladybrown_state == 0)
    {
      // if (ladybrown_sensor.get_position() > 0)
      // ladybrown.move(ladybrown_pid.update(-100, lb_pos));
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_COAST);
      if (!ladybrown_oncer)
        ladybrown_motor.move_absolute(-5 + lb_offset, 200);
      ladybrown_oncer = true;
    }
    else
      ladybrown_oncer = false;
    if (ladybrown_state == 1)
    {
      // ladybrown.move(ladybrown_pid.update(-95, lb_pos));
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
      ladybrown_motor.move_absolute(-95 + lb_offset, 200);
    }
    else if (ladybrown_state == 2)
    {
      // ladybrown.move(ladybrown_pid.update(0, lb_pos));
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
      ladybrown_motor.move_absolute(-650 + lb_offset, 100);
    }
    else if (ladybrown_state == 3)
    {
      // ladybrown.move(ladybrown_pid.update(70, lb_pos));
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
      ladybrown_motor.move_absolute(-0 + lb_offset, 200);
    }

    if (master.get_digital(DIGITAL_R1))
    {
      // chain.move_velocity(chain_speed);
      roller.move(127);
    }
    else if (master.get_digital(DIGITAL_R2))
    {
      // chain.move_velocity(-chain_speed);
      roller.move(-127);
    }
    else
    {
      // chain.move(0);
      roller.move(0);
    }

    //* DRIVE CODE
    double forward = master.get_analog(ANALOG_LEFT_Y),
           turn = master.get_analog(ANALOG_RIGHT_X) *
                  (0.01 * turning_speed);

    double left = forward + turn, right = forward - turn;
    double max_val = max(127.0, max(abs(left), abs(right)));
    left /= max_val;
    right /= max_val;

    // if (!master.get_digital(E_CONTROLLER_DIGITAL_R1))
    // {

    // continue;

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