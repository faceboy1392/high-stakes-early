#include "main.h"
#include "liblvgl/core/lv_obj.h"
#include "pros/apix.h"

#include <algorithm>
#include <math.h>
#include <string.h>

// #include "ControllerUI.h"
#include "devices.h"
#include "pros/screen.h"
#include "pros/screen.hpp"

// #include "PID.h"

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

unsigned long createRGB(int r, int g, int b)
{
  return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}
unsigned long createRGBA(int r, int g, int b, int a)
{
  return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) +
         (a & 0xff);
}

/*
 *  _____                              ___
 * | ____|_ __  _   _ _ __ ___  ___   ( _ )
 * |  _| | '_ \| | | | '_ ` _ \/ __|  / _ \/\
 * | |___| | | | |_| | | | | | \__ \ | (_>  <
 * |_____|_| |_|\__,_|_| |_| |_|___/  \___/\/
 * \ \   / /_ _ _ __(_) __ _| |__ | | ___  ___
 *  \ \ / / _` | '__| |/ _` | '_ \| |/ _ \/ __|
 *   \ V / (_| | |  | | (_| | |_) | |  __/\__ \
 *    \_/ \__,_|_|  |_|\__,_|_.__/|_|\___||___/
 */

//* Competition status
enum CompStatus
{
  DISABLED,
  AUTONOMOUS,
  OPCONTROL,
  DISCONNECTED
};

CompStatus get_competition_status()
{
  if (!competition::is_connected())
    return DISCONNECTED;
  if (competition::is_autonomous())
    return AUTONOMOUS;
  if (competition::is_disabled())
    return DISABLED;
  return OPCONTROL;
}

//* Autonomous selection

enum AutonRoutine
{
  AUTON_NONE,
  AUTON_FORWARD,
  AUTON_BASIC,
  AUTON_AGGRESSIVE,
  AUTON_DEFENSIVE,
  AUTON_SKILLS
};
enum FieldSide
{
  /**
   * red LEFT side
   */
  RED_POSITIVE,
  /**
   * red RIGHT side
   */
  RED_NEGATIVE,
  /**
   * blue LEFT side
   */
  BLUE_POSITIVE,
  /**
   * blue RIGHT side
   */
  BLUE_NEGATIVE
};
enum LeftOrRight
{
  LEFT,
  RIGHT
};
enum PositiveOrNegative
{
  POSITIVE,
  NEGATIVE
};
enum AllianceColor
{
  ALLIANCE_RED,
  ALLIANCE_BLUE,
  ALLIANCE_NEITHER // comes in handy for color sort
};

AutonRoutine selected_auton = AUTON_FORWARD;
FieldSide selected_field_side = RED_POSITIVE; // arbitrary default
bool auton_selector_active = false;

//* Color sorting
// enum AllianceColor
// {
//   ALLIANCE_RED,
//   ALLIANCE_BLUE,
//   ALLIANCE_NEITHER,
// };

AllianceColor ring_color_to_eject = ALLIANCE_NEITHER;

/**
 * for reliable, colorblind ring ejection
 */
bool detected_ring = false;
AllianceColor last_detected_ring_color = ALLIANCE_NEITHER;

int ring_detection_reset_ticker = 0;

/**
 * @deprecated im gonna make this system better
 */
int color_ticker = 0;

// color averaging
int color_hue_sum = 0;
int color_avg_ticks = 0;
int color_avg_fluke_ticker = 0; // don't reset the avg until you are sure the ring is gone

int previous_ejection_motor_pos = 0;

//* Ladybrown
/**
 * 1 - stow
 * 2 - prime
 * 3 - extend
 */
int ladybrown_state;

int lb_pos_stow = -5,
    lb_pos_prime = -100,
    lb_pos_extend = -650;

/**
 * in degrees, to account for starting the program with the ladybrown in a different position
 */
double ladybrown_offset = 0;

//* Intake
/**
 * -1 reverse
 * 0 stop
 * 1 intake
 * 2 always eject
 * 3 slow speed
 */
int intake_direction = 0;

/*
 *  _   _      _
 * | | | | ___| |_ __   ___ _ __
 * | |_| |/ _ \ | '_ \ / _ \ '__|
 * |  _  |  __/ | |_) |  __/ |
 * |_|_|_|\___|_| .__/ \___|_|_
 * |  ___|   _ _|_|   ___| |_(_) ___  _ __  ___
 * | |_ | | | | '_ \ / __| __| |/ _ \| '_ \/ __|
 * |  _|| |_| | | | | (__| |_| | (_) | | | \__ \
 * |_|   \__,_|_| |_|\___|\__|_|\___/|_| |_|___/
 */

LeftOrRight left_or_right(FieldSide side)
{
  if (side == RED_NEGATIVE || side == BLUE_POSITIVE)
    return LEFT;
  else if (side == RED_POSITIVE || side == BLUE_NEGATIVE)
    return RIGHT;
}
PositiveOrNegative positive_or_negative(FieldSide side)
{
  if (side == RED_POSITIVE || side == BLUE_POSITIVE)
    return POSITIVE;
  else if (side == RED_NEGATIVE || side == BLUE_NEGATIVE)
    return NEGATIVE;
}
AllianceColor red_or_blue(FieldSide side)
{
  if (side == RED_POSITIVE || side == RED_NEGATIVE)
    return ALLIANCE_RED;
  else if (side == BLUE_POSITIVE || side == BLUE_NEGATIVE)
    return ALLIANCE_BLUE;
}
AllianceColor invert_red_blue(AllianceColor color)
{
  if (color == ALLIANCE_RED)
    return ALLIANCE_BLUE;
  else if (color == ALLIANCE_BLUE)
    return ALLIANCE_RED;
  return ALLIANCE_NEITHER;
}

void brain_screen()
{

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
  /*
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

void print_to_controller(string line1, string line2, string line3)
{
  master.print(0, 0, line1.c_str());
  delay(51);
  master.print(1, 0, line2.c_str());
  delay(51);
  master.print(2, 0, line3.c_str());
  delay(51);
}

void detect_ring_color()
{
  // blue 210
  // red 20 ish

  ring_detection_reset_ticker++;

  if (ring_distance.get_distance() < 20)
  {
    color_hue_sum += ring_detector.get_hue();
    color_avg_ticks++;

    color_avg_fluke_ticker = 0;

    ring_detection_reset_ticker = 0;
  }
  else
  {
    color_avg_fluke_ticker++;
  }

  if (color_avg_ticks > 3)
  {
    if (color_hue_sum / color_avg_ticks > 50)
    {
      detected_ring = true;
      last_detected_ring_color = ALLIANCE_BLUE;
      screen::set_pen(Color::blue);
      screen::fill_rect(0, 0, 480, 240);
    }
    else
    {
      detected_ring = true;
      last_detected_ring_color = ALLIANCE_RED;
      screen::set_pen(Color::red);
      screen::fill_rect(0, 0, 480, 240);
    }
  }

  if (ring_detection_reset_ticker > 15)
  {
    last_detected_ring_color = ALLIANCE_NEITHER;
  }

  if (color_avg_fluke_ticker >= 5)
  {
    screen::erase_rect(120, 60, 360, 180);
    color_hue_sum = 0;
    color_avg_ticks = 0;
  }

  /*
  // master.print(0, 0, to_string(ring_detector.get_hue()).c_str());
  if (ring_distance.get_distance() < 30 && color_ticker < 6)
    color_ticker++;
  else
    color_ticker = 0;

  if (color_ticker >= 6)
  {
    detected_ring = true;
    int hue = ring_detector.get_hue();
    // if (hue > 180 && hue < 280)
    if (hue > 120)
    {
      last_detected_ring_color = ALLIANCE_BLUE;
      ring_detection_reset_ticker = 0;
      screen::set_pen(Color::blue);
      screen::fill_rect(0, 0, 480, 240);
    }
    // else if (hue > 0 && hue < 50)
    else if (hue < 70)
    {
      last_detected_ring_color = ALLIANCE_RED;
      ring_detection_reset_ticker = 0;
      screen::set_pen(Color::red);
      screen::fill_rect(0, 0, 480, 240);
    }
  }
  */

  // make sure to update this if i update the delay timer
  if (ring_detection_reset_ticker >= 15)
  {
    last_detected_ring_color = ALLIANCE_NEITHER;
    detected_ring = false;
    // screen::erase();
    // white rectangle in the middle
    screen::set_pen(Color::white);
    screen::fill_rect(120, 60, 360, 180);
    // master.clear();
  }
}

void universal_intake_controller()
{
  int unjam_ticker = 0;
  int eject_ticker = 0;

  double prev_chain_pos = chain.get_position();
  double prev_eject_pos = -1000000; // arbitrary value

  while (true)
  {
    prev_chain_pos = chain.get_position();
    delay(10);

    //* EJECT ACTIVATION
    bool eject = false;

    // EJECTION CHAIN DETECTION
    bool in_position_to_eject = false;
    if (chain_detector.get_distance() < 30 && eject_ticker >= 10)
    {
      in_position_to_eject = true;
      prev_eject_pos = chain.get_position();
      // master.rumble(".");
    }

    // EJECTION GAP FILLING
    double chain_diff = abs(chain.get_position() - prev_eject_pos) / 360.0;
    // double chain_diff_mod = fmod(chain_diff, 4.0);
    // if (chain_diff_mod >= 3.9 || chain_diff_mod <= 0.1)
    if (chain_diff >= 3.99)
    {
      in_position_to_eject = true;
      prev_eject_pos -= 4.0 * 360 * signbit(chain_diff);
    }

    // must be calibrated by distance sensor to override the default value before it can be used
    if (prev_eject_pos == -1000000)
      in_position_to_eject = false;

    // if (in_position_to_eject)
    // {
    //   master.rumble(".");
    // }

    // COLOR SORT
    detect_ring_color();
    bool correct_color = (last_detected_ring_color == ring_color_to_eject) && (ring_color_to_eject != ALLIANCE_NEITHER);

    // holding both buttons force eject
    eject = in_position_to_eject &&
            (correct_color ||
             (intake_direction == 2 &&
              detected_ring // there should still be a ring there to eject
              ));

    //* CONTROL
    if (intake_direction == 3)
    {
      chain.move_velocity(400);
    }
    else if (intake_direction >= 1)
    {
      chain.move_velocity(600);
    }
    else if (intake_direction == -1)
    {
      chain.move_velocity(-600);
    }
    else
    {
      chain.move_velocity(0);
      unjam_ticker = 0;
    }

    //* UNJAMMER
    // (boldly) assuming this loop is PERFECTLY (and impossibly) fast with its 10msec delay,
    // the chain should still move 3.6 degrees per cycle if the motor is at max speed
    if (abs(chain.get_position() - prev_chain_pos) < 0.5 && unjam_ticker >= 15)
    {
      unjam_ticker = 0;
      master.rumble("-");
      chain.move_velocity(-600);
      delay(200);
    }
    else if (unjam_ticker < 15)
      unjam_ticker++;

    //* EJECTOR
    if (eject && (eject_ticker >= 15) && (ladybrown_state != 1))
    {
      last_detected_ring_color = ALLIANCE_NEITHER;
      detected_ring = false;

      unjam_ticker = 0;
      eject_ticker = 0;
      master.rumble(".");
      chain.set_brake_mode(MOTOR_BRAKE_BRAKE);
      chain.brake();
      delay(200);
      chain.set_brake_mode(MOTOR_BRAKE_COAST);
    }
    else if (eject_ticker < 15)
      eject_ticker++;
  }
}
Task unjammer = Task(universal_intake_controller);

void controller_screen()
{
  while (true)
  {
    delay(10);

    string line1 = "";
    string line2 = "";
    string line3 = "";

    print_to_controller(line1, line2, line3);
  }
}

/*
 *     _         _
 *    / \  _   _| |_ ___  _ __
 *   / _ \| | | | __/ _ \| '_ \
 *  / ___ \ |_| | || (_) | | | |
 * /_/__ \_\__,_|\__\___/|_| |_|
 * |  _ \ ___  _   _| |_(_)_ __   ___  ___
 * | |_) / _ \| | | | __| | '_ \ / _ \/ __|
 * |  _ < (_) | |_| | |_| | | | |  __/\__ \
 * |_| \_\___/ \__,_|\__|_|_| |_|\___||___/
 */

/**
 * a shorthand for `chassis.waitUntilDone()`
 */
void wait()
{
  chassis.waitUntilDone();
}

//* Simple autons
void auton_drive_forward()
{
  chassis.tank(30, 30);
  delay(800);
  chassis.tank(0, 0);
}

void auton_basic(FieldSide side)
{
  int r = left_or_right(side) == RIGHT ? 1 : -1;

  chassis.setPose({r * 24, 7.5 + 0, 0}, false);

  roller.move(127);
  hook.set_value(true);
  delay(500);

  //* stack adjacent to the goal
  chassis.moveToPoint(r * (48 + 2), 48 + 2, 1000);
  wait();

  hook.set_value(false);
  delay(400);

  chassis.turnToPoint(r * 0, 48, 1000, {.forwards = false});
  wait();
  chassis.moveToPoint(r * (24 + 0), 48, 2000, {.forwards = false, .maxSpeed = 45});
  wait();

  //* grab goal
  // delay(100);
  lever.set_value(true);
  intake_direction = 1;

  delay(2000);

  chassis.turnToPoint(r * -100, 48, 1000);
  chassis.moveToPoint(r * 11, 48, 1500, {.maxSpeed = 60});
  chassis.waitUntilDone();

  roller.brake();
  intake_direction = 0;
}

ASSET(path_rpa1_txt);
ASSET(path_rpa2_txt);
//* Advanced autons
void auton_positive_aggressive(FieldSide side)
{
  // int r = left_or_right(side) == RIGHT ? 1 : -1;

  chassis.setPose({-63, -24, 90}, false);

  roller.move(127);

  chassis.follow(path_rpa1_txt, 15, 4000);
  chassis.follow(path_rpa2_txt, 4, 2000, false);
  wait();

  lever.set_value(true);
  intake_direction = 1;
}

void auton_positive_defensive(FieldSide side)
{
}

void auton_negative_aggressive(FieldSide side)
{
}

void auton_negative_defensive(FieldSide side, bool mirror = false)
{
  int r = mirror ? -1 : 1;

  chassis.setPose({r * 24, 7.5 + 2, 0}, false);

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
  // Task intake_unjammer = Task(autonIntakeUnjammer);
  intake_direction = 1;

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

//* Auton skills
void auton_skills()
{
  chassis.setBrakeMode(MOTOR_BRAKE_BRAKE);

  chassis.setPose({0, 24 - 8 + 0.5, 180}, false);

  ring_color_to_eject = ALLIANCE_NEITHER;
  /*
  while (true) {
    delay(51);
    // print in one print statement the x and y pos and rotation of the robot
    master.print(0, 0, to_string(chassis.getPose().x).c_str());
    delay(51);
    master.print(1, 0, to_string(chassis.getPose().y).c_str());
    delay(51);
    master.print(2, 0, to_string(chassis.getPose().theta).c_str());
  }
  */

  // alliance stake
  chassis.moveToPoint(0, 24 - 8 - 7, 1000);
  wait();

  ladybrown_motor.move_absolute(-320, 200);
  delay(500);

  // back up
  chassis.moveToPoint(0, 24, 1000, {.forwards = false});
  wait();

  ladybrown_motor.move_absolute(-10, 200);
  ladybrown_motor.set_brake_mode(MOTOR_BRAKE_COAST);

  int d = 1;

  while (true)
  {

    // turn to first goal
    chassis.turnToHeading(270, 1200);
    chassis.moveToPoint(d*(24 + 3), 24 - 1, 3000, {.forwards = false, .maxSpeed = 80});
    // chassis.moveToPose(24, 24, 270, 2000, {.forwards = false, .maxSpeed = 127});
    wait();

    // delay(150);
    lever.set_value(true);
    delay(350);

    intake_direction = 1;
    roller.move(127);

    // first two rings in a row
    chassis.turnToHeading(90, 1000);
    chassis.moveToPoint(d*(48 + 8), 24, 2000, {.maxSpeed = 80});
    chassis.moveToPoint(d*48, 24 + 8, 800, {.forwards = false});
    // side ring
    chassis.turnToPoint(d*48, 12, 2000, {.maxSpeed = 60, .minSpeed = 20, .earlyExitRange = 5});
    chassis.moveToPoint(d*46, 18, 800);
    wait();
    delay(200);
    chassis.turnToPoint(d*24, 48, 800);
    chassis.moveToPoint(d*24, 48, 1500);
    chassis.turnToPoint(d*48, 48, 1500, {.maxSpeed = 60});
    chassis.moveToPoint(d*48, 48, 1000);

    // to the wall stake
    chassis.turnToPoint(d*48 - 7, 72, 1000);
    chassis.moveToPoint(d*48 - 9, 72-2, 2000, {.maxSpeed = 70});
    wait();
    chassis.turnToPoint(d*72, 72-2, 1000);

    ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
    ladybrown_motor.move_absolute(lb_pos_prime + ladybrown_offset, 200);

    chassis.moveToPoint(d*(72 - 10.5 - 4.5), 72-2, 1200, {.maxSpeed = 50});
    chassis.turnToHeading(90, 1000);
    wait();
    delay(800);

    // 1st tall wall stake ring
    ladybrown_motor.move_absolute(lb_pos_extend + ladybrown_offset, 200);
    delay(1000);
    chassis.turnToHeading(90+30, 800);
    chassis.turnToHeading(90-30, 1200);
    chassis.turnToHeading(90, 800);
    chassis.moveToPoint(72, 72, 400);
    wait();

    chassis.moveToPoint(d*(48 - 7), 72, 1000, {.forwards = false});
    ladybrown_motor.move_absolute(lb_pos_stow + ladybrown_offset, 200);
    wait();

    chassis.turnToPoint(d*48, 72 + 24, 1000);
    chassis.moveToPoint(d*48, 72 + 24, 1500, {.maxSpeed = 80});
    chassis.turnToPoint(d*24, 72 + 24, 1000, {.maxSpeed = 60});
    wait();
    delay(500); // let the ring get on the goal

    chassis.moveToPoint(d*24, 72 + 24, 1000);
    wait();
    chassis.turnToPoint(d*(48 - 7), 72, 1200); // let it be async
    ladybrown_motor.move_absolute(lb_pos_prime + ladybrown_offset, 200);
    chassis.moveToPoint(d*(48 - 7), 72, 1500, {.forwards = false, .maxSpeed = 70});

    delay(1000);

    chassis.turnToPoint(d*72, 72, 1000);
    chassis.moveToPoint(d*(72 - 10.5 - 3), 72, 1000, {.maxSpeed = 40});
    chassis.turnToHeading(90, 1000);
    wait();

    ladybrown_motor.move_absolute(lb_pos_extend + ladybrown_offset, 200);
    delay(1000);
    chassis.turnToHeading(90+30, 800);
    chassis.turnToHeading(90-30, 1200);
    chassis.turnToHeading(90, 800);
    chassis.moveToPoint(72, 72, 400);
    wait();

    chassis.moveToPoint(d*(48 - 7), 72, 1000, {.forwards = false});
    wait();
    ladybrown_motor.move_absolute(lb_pos_stow + ladybrown_offset, 200);

    // put the goal in the corner lil bro
    chassis.moveToPoint(d*60, 18, 2000, {.forwards = false});
    wait();

    lever.set_value(false);

    break;
    if (d == 1)
    {
      d = -1;
      continue;
    }
    else
      break;
  }
}

/*
 *  _     _  __                      _
 * | |   (_)/ _| ___  ___ _   _  ___| | ___
 * | |   | | |_ / _ \/ __| | | |/ __| |/ _ \
 * | |___| |  _|  __/ (__| |_| | (__| |  __/
 * |_____|_|_|  \___|\___|\__, |\___|_|\___|
 *  _____                 |___/
 * |  ___|   _ _ __   ___| |_(_) ___  _ __  ___
 * | |_ | | | | '_ \ / __| __| |/ _ \| '_ \/ __|
 * |  _|| |_| | | | | (__| |_| | (_) | | | \__ \
 * |_|   \__,_|_| |_|\___|\__|_|\___/|_| |_|___/
 */
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
  ladybrown_offset = ladybrown_state == 1 ? lb_sensor_pos * 5 : 0;

  delay(50);
  // master.print(0, 0, "Calibrating...");
  // partner.print(0, 0, "Calibrating...");

  // calibration

  // controller task
  // Task controller_task(controllerAutonSelector);

  ring_detector.set_led_pwm(100);
  ring_detector.disable_gesture();

  // brain screen task
  Task brain_task(brain_screen);
}

void disabled() {}

void competition_initialize() {}

void autonomous()
{
  // chassis is already calibrated in initialize()

  left_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  right_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  auton_selector_active = false;

  //! HARDCODER
  // selected_auton =
  // selected_auton = AUTON_BASIC;
  selected_auton = AUTON_AGGRESSIVE;
  // selected_auton = AUTON_DEFENSIVE;

  selected_field_side = RED_POSITIVE;
  // selected_field_side = RED_NEGATIVE;
  // selected_field_side = BLUE_POSITIVE;
  // selected_field_side = BLUE_NEGATIVE;

  // selected_auton = AUTON_SKILLS;

  // ring_color_to_eject = invert_red_blue(red_or_blue(selected_field_side));
  ring_color_to_eject = ALLIANCE_NEITHER;

  switch (selected_auton)
  {
  //* symmetrical autons
  case AUTON_NONE:
    break;

  case AUTON_FORWARD:
    auton_drive_forward();
    break;

  //* asymmetrical autons
  case AUTON_BASIC:
    auton_basic(selected_field_side);
    break;

  case AUTON_AGGRESSIVE:
    if (positive_or_negative(selected_field_side) == POSITIVE)
      auton_positive_aggressive(selected_field_side);
    else
      auton_negative_aggressive(selected_field_side);
    break;

  case AUTON_DEFENSIVE:
    if (positive_or_negative(selected_field_side) == POSITIVE)
      auton_positive_defensive(selected_field_side);
    else
      auton_negative_defensive(selected_field_side);
    break;

  //* skills
  case AUTON_SKILLS:
    ring_color_to_eject = ALLIANCE_BLUE;
    auton_skills();
    break;

  default:
    break;
  }

  return;
}

/*
 *  ____       _                           _
 * |  _ \ _ __(_)_   _____    ___ ___   __| | ___
 * | | | | '__| \ \ / / _ \  / __/ _ \ / _` |/ _ \
 * | |_| | |  | |\ V /  __/ | (_| (_) | (_| |  __/
 * |____/|_|  |_| \_/ \___|  \___\___/ \__,_|\___|
 */

void opcontrol()
{
  delay(51);
  master.clear();

  auton_selector_active = false;

  left_motors.set_brake_mode(MOTOR_BRAKE_COAST);
  right_motors.set_brake_mode(MOTOR_BRAKE_COAST);

  int turning_speed = 60;

  bool ladybrown_oncer = false;
  int ladybrown_haptic_ticker = 0;

  if (red_or_blue(selected_field_side) == ALLIANCE_RED)
  {
    ring_color_to_eject = ALLIANCE_BLUE;
  }
  else if (red_or_blue(selected_field_side) == ALLIANCE_BLUE)
  {
    ring_color_to_eject = ALLIANCE_RED;
  }

  ring_detector.set_led_pwm(100);

  int tick = 0;

  while (true)
  {
    delay(20);
    tick++;

    if (false)
      continue; // DISABLE THE DRIVE LOOP SO I DONT ACCIDENTALLY DRIVE IT OFF
                // THE TABLE

    detect_ring_color();

    //* clamp
    if (master.get_digital(DIGITAL_L1))
      lever.set_value(true);
    else if (master.get_digital(DIGITAL_L2))
      lever.set_value(false);

    //* arm
    if (master.get_digital(DIGITAL_UP))
      hook.set_value(false);
    else if (master.get_digital(DIGITAL_DOWN))
      hook.set_value(true);

    //* set eject color
    if (master.get_digital_new_press(DIGITAL_LEFT))
    {
      if (ring_color_to_eject == ALLIANCE_RED)
      {

        master.print(1, 0, "EJECT BLUE");
        ring_color_to_eject = ALLIANCE_BLUE;
      }
      else if (ring_color_to_eject == ALLIANCE_BLUE)
      {

        master.print(1, 0, "EJECT RED");
        ring_color_to_eject = ALLIANCE_RED;
      }
      else
      {
        master.print(1, 0, "EJECT RED");
        ring_color_to_eject = ALLIANCE_RED;
      }
    }
    if (master.get_digital_new_press(DIGITAL_RIGHT))
    {
      master.print(1, 0, "NO EJECT");
      ring_color_to_eject = ALLIANCE_NEITHER;
    }

    //* ladybrown
    if (master.get_digital(DIGITAL_B))
      ladybrown_state = 0;
    else if (master.get_digital(DIGITAL_Y) ||
             master.get_digital(DIGITAL_A))
      ladybrown_state = 1;
    else if (master.get_digital(DIGITAL_X))
      ladybrown_state = 2;

    //* ladybrown stuff
    ladybrown_haptic_ticker++;
    if (ladybrown_haptic_ticker > 50 && ladybrown_state == 1)
    {
      master.rumble(".");
      ladybrown_haptic_ticker = 0;
    }

    if (ladybrown_state == 0)
    {
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_COAST);
      if (!ladybrown_oncer)
        ladybrown_motor.move_absolute(lb_pos_stow + ladybrown_offset, 200);
      ladybrown_oncer = true;
    }
    else
    {
      ladybrown_oncer = false;
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
    }
    if (ladybrown_state == 1)
      ladybrown_motor.move_absolute(lb_pos_prime + ladybrown_offset, 200);
    else if (ladybrown_state == 2)
      ladybrown_motor.move_absolute(lb_pos_extend + ladybrown_offset, 100);

    //* intake
    if (master.get_digital(DIGITAL_R1))
    {
      if (master.get_digital(DIGITAL_R2))
      {
        intake_direction = 2;
      }
      else
      {
        intake_direction = 1;
      }
      roller.move(127);
    }
    else if (master.get_digital(DIGITAL_R2))
    {
      roller.move(-127);
      intake_direction = -1;
    }
    else
    {
      roller.move(0);
      intake_direction = 0;
    }

    //* drive code
    double forward = master.get_analog(ANALOG_LEFT_Y),
           turn = master.get_analog(ANALOG_RIGHT_X) *
                  (0.01 * turning_speed);

    double left = forward + turn, right = forward - turn;
    double max_val = max(127.0, max(abs(left), abs(right)));
    left /= max_val;
    right /= max_val;

    left_motors.move(left * 127);
    right_motors.move(right * 127);
  }
}