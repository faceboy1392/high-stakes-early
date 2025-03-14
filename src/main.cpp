#include "main.h"
#include "liblvgl/core/lv_obj.h"
#include "pros/apix.h"
#include "pros/screen.h"
#include "pros/screen.hpp"

#include <algorithm>
#include <math.h>
#include <string.h>
#include <map>

// #include "ControllerUI.h"
#include "devices.h"

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

// to count competition times and whatever
int timer_offset = 0;

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
  AUTON_LINE,
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

struct AutonMapKey
{
  AutonRoutine first;
  FieldSide second;

  bool operator<(const AutonMapKey &other) const
  {
    if (first != other.first)
    {
      return first < other.first;
    }
    return second < other.second;
  }
};

AutonRoutine selected_auton = AUTON_LINE;
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
    lb_pos_prime = -90,
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

bool threshold(double x, double min, double max) { return x >= min && x <= max; }

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
      // screen::set_pen(Color::blue);
      // screen::fill_rect(0, 0, 480, 240);
    }
    else
    {
      detected_ring = true;
      last_detected_ring_color = ALLIANCE_RED;
      // screen::set_pen(Color::red);
      // screen::fill_rect(0, 0, 480, 240);
    }
  }

  if (ring_detection_reset_ticker > 15)
  {
    last_detected_ring_color = ALLIANCE_NEITHER;
  }

  if (color_avg_fluke_ticker >= 5)
  {
    // screen::erase_rect(120, 60, 360, 180);
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
    // screen::set_pen(Color::white);
    // screen::fill_rect(120, 60, 360, 180);
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
      delay(100);
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

/*
 *  ____  _           _
 * |  _ \(_)___ _ __ | | __ _ _   _
 * | | | | / __| '_ \| |/ _` | | | |
 * | |_| | \__ \ |_) | | (_| | |_| |
 * |____/|_|___/ .__/|_|\__,_|\__, |
 *             |_|            |___/
 */

void brain_screen()
{
  LV_IMG_DECLARE(match_field);

  lv_obj_t *img = lv_img_create(lv_scr_act());
  lv_img_set_src(img, &match_field);
  lv_obj_set_pos(img, 120, 0);
  lv_obj_set_size(img, 240, 240);

  // 480x240 wxh screen
  // red negative top left,
  // blue negative top right,
  // red positive bottom left,
  // blue positive bottom right

  // each corner gets 2 rectangular spaces to click to select between aggressive (top choice) and defensive (bottom choice)
  // there are also 2 buttons in the middle left and right, for none and skills respectively. that means each side is divided into 5 rectangles.
  // those two extra rectangles can take a third of each of their side, while the other four on each side take a sixth each (in height)
  // these must also be labeled "Offensive", "Defensive", "None", and "Skills"

  // top left
  lv_obj_t *red_neg_aggressive = lv_obj_create(lv_scr_act());
  lv_obj_set_size(red_neg_aggressive, 120, 40);
  lv_obj_set_pos(red_neg_aggressive, 0, 0);
  lv_obj_set_style_radius(red_neg_aggressive, 0, 0);
  lv_obj_set_style_pad_all(red_neg_aggressive, 0, 0);
  lv_obj_set_style_bg_color(red_neg_aggressive, lv_color_make(235, 25, 25), 0);

  lv_obj_t *red_neg_defensive = lv_obj_create(lv_scr_act());
  lv_obj_set_size(red_neg_defensive, 120, 40);
  lv_obj_set_pos(red_neg_defensive, 0, 40);
  lv_obj_set_style_radius(red_neg_defensive, 0, 0);
  lv_obj_set_style_pad_all(red_neg_defensive, 0, 0);
  lv_obj_set_style_bg_color(red_neg_defensive, lv_color_make(235, 25, 25), 0);

  // top left labels
  lv_obj_t *red_neg_aggressive_label = lv_label_create(red_neg_aggressive);
  lv_label_set_text(red_neg_aggressive_label, "Aggressive");
  lv_obj_align(red_neg_aggressive_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(red_neg_aggressive_label, lv_color_make(255, 255, 255), 0);

  lv_obj_t *red_neg_defensive_label = lv_label_create(red_neg_defensive);
  lv_label_set_text(red_neg_defensive_label, "Defensive");
  lv_obj_align(red_neg_defensive_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(red_neg_defensive_label, lv_color_make(255, 255, 255), 0);

  // top right
  lv_obj_t *blue_neg_aggressive = lv_obj_create(lv_scr_act());
  lv_obj_set_size(blue_neg_aggressive, 120, 40);
  lv_obj_set_pos(blue_neg_aggressive, 360, 0);
  lv_obj_set_style_radius(blue_neg_aggressive, 0, 0);
  lv_obj_set_style_pad_all(blue_neg_aggressive, 0, 0);
  lv_obj_set_style_bg_color(blue_neg_aggressive, lv_color_make(25, 160, 228), 0);

  lv_obj_t *blue_neg_defensive = lv_obj_create(lv_scr_act());
  lv_obj_set_size(blue_neg_defensive, 120, 40);
  lv_obj_set_pos(blue_neg_defensive, 360, 40);
  lv_obj_set_style_radius(blue_neg_defensive, 0, 0);
  lv_obj_set_style_pad_all(blue_neg_defensive, 0, 0);
  lv_obj_set_style_bg_color(blue_neg_defensive, lv_color_make(25, 160, 228), 0);

  // top right labels
  lv_obj_t *blue_neg_aggressive_label = lv_label_create(blue_neg_aggressive);
  lv_label_set_text(blue_neg_aggressive_label, "Aggressive");
  lv_obj_align(blue_neg_aggressive_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(blue_neg_aggressive_label, lv_color_make(255, 255, 255), 0);

  lv_obj_t *blue_neg_defensive_label = lv_label_create(blue_neg_defensive);
  lv_label_set_text(blue_neg_defensive_label, "Defensive");
  lv_obj_align(blue_neg_defensive_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(blue_neg_defensive_label, lv_color_make(255, 255, 255), 0);

  // bottom left
  lv_obj_t *red_pos_aggressive = lv_obj_create(lv_scr_act());
  lv_obj_set_size(red_pos_aggressive, 120, 40);
  lv_obj_set_pos(red_pos_aggressive, 0, 200);
  lv_obj_set_style_radius(red_pos_aggressive, 0, 0);
  lv_obj_set_style_pad_all(red_pos_aggressive, 0, 0);
  lv_obj_set_style_bg_color(red_pos_aggressive, lv_color_make(235, 25, 25), 0);

  lv_obj_t *red_pos_defensive = lv_obj_create(lv_scr_act());
  lv_obj_set_size(red_pos_defensive, 120, 40);
  lv_obj_set_pos(red_pos_defensive, 0, 160);
  lv_obj_set_style_radius(red_pos_defensive, 0, 0);
  lv_obj_set_style_pad_all(red_pos_defensive, 0, 0);
  lv_obj_set_style_bg_color(red_pos_defensive, lv_color_make(235, 25, 25), 0);

  // bottom left labels
  lv_obj_t *red_pos_aggressive_label = lv_label_create(red_pos_aggressive);
  lv_label_set_text(red_pos_aggressive_label, "Aggressive");
  lv_obj_align(red_pos_aggressive_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(red_pos_aggressive_label, lv_color_make(255, 255, 255), 0);

  lv_obj_t *red_pos_defensive_label = lv_label_create(red_pos_defensive);
  lv_label_set_text(red_pos_defensive_label, "Defensive");
  lv_obj_align(red_pos_defensive_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(red_pos_defensive_label, lv_color_make(255, 255, 255), 0);

  // bottom right
  lv_obj_t *blue_pos_aggressive = lv_obj_create(lv_scr_act());
  lv_obj_set_size(blue_pos_aggressive, 120, 40);
  lv_obj_set_pos(blue_pos_aggressive, 360, 200);
  lv_obj_set_style_radius(blue_pos_aggressive, 0, 0);
  lv_obj_set_style_pad_all(blue_pos_aggressive, 0, 0);
  lv_obj_set_style_bg_color(blue_pos_aggressive, lv_color_make(25, 160, 228), 0);

  lv_obj_t *blue_pos_defensive = lv_obj_create(lv_scr_act());
  lv_obj_set_size(blue_pos_defensive, 120, 40);
  lv_obj_set_pos(blue_pos_defensive, 360, 160);
  lv_obj_set_style_radius(blue_pos_defensive, 0, 0);
  lv_obj_set_style_pad_all(blue_pos_defensive, 0, 0);
  lv_obj_set_style_bg_color(blue_pos_defensive, lv_color_make(25, 160, 228), 0);

  // bottom right labels
  lv_obj_t *blue_pos_aggressive_label = lv_label_create(blue_pos_aggressive);
  lv_label_set_text(blue_pos_aggressive_label, "Aggressive");
  lv_obj_align(blue_pos_aggressive_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(blue_pos_aggressive_label, lv_color_make(255, 255, 255), 0);

  lv_obj_t *blue_pos_defensive_label = lv_label_create(blue_pos_defensive);
  lv_label_set_text(blue_pos_defensive_label, "Defensive");
  lv_obj_align(blue_pos_defensive_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(blue_pos_defensive_label, lv_color_make(255, 255, 255), 0);

  // middle left
  // none is a dark mode box, skills is light mode
  // ok none now shares its space with AUTON_LINE, "Back up"
  lv_obj_t *none = lv_obj_create(lv_scr_act());
  lv_obj_set_size(none, 120, 40);
  lv_obj_set_pos(none, 0, 80);
  lv_obj_set_style_radius(none, 0, 0);
  lv_obj_set_style_pad_all(none, 0, 0);
  lv_obj_set_style_bg_color(none, lv_color_make(0, 0, 0), 0);

  lv_obj_t *none_label = lv_label_create(none);
  lv_label_set_text(none_label, "None");
  lv_obj_align(none_label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *back_up = lv_obj_create(lv_scr_act());
  lv_obj_set_size(back_up, 120, 40);
  lv_obj_set_pos(back_up, 0, 120);
  lv_obj_set_style_radius(back_up, 0, 0);
  lv_obj_set_style_pad_all(back_up, 0, 0);
  lv_obj_set_style_bg_color(back_up, lv_color_make(0, 0, 0), 0);

  lv_obj_t *back_up_label = lv_label_create(back_up);
  lv_label_set_text(back_up_label, "Back up");
  lv_obj_align(back_up_label, LV_ALIGN_CENTER, 0, 0);

  // middle right
  lv_obj_t *skills = lv_obj_create(lv_scr_act());
  lv_obj_set_size(skills, 120, 80);
  lv_obj_set_pos(skills, 360, 80);
  lv_obj_set_style_radius(skills, 0, 0);
  lv_obj_set_style_pad_all(skills, 0, 0);
  lv_obj_set_style_bg_color(skills, lv_color_make(175, 175, 175), 0);

  lv_obj_t *skills_label = lv_label_create(skills);
  lv_label_set_text(skills_label, "Skills");
  lv_obj_align(skills_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(skills_label, lv_color_make(0, 0, 0), 0);

  // list/dictionary with each auton type AND side and its corresponding string name and a set of two coordinates to draw a box around it on the screen
  struct AutonOption
  {
    string name;
    int selectionX1;
    int selectionY1;
    int selectionX2;
    int selectionY2;
  };

  map<AutonMapKey, AutonOption> auton_options;

  auton_options[{AUTON_NONE, RED_POSITIVE}] = {"None", 0, 80, 120, 120};
  auton_options[{AUTON_NONE, RED_NEGATIVE}] = {"None", 0, 80, 120, 120};
  auton_options[{AUTON_NONE, BLUE_POSITIVE}] = {"None", 0, 80, 120, 120};
  auton_options[{AUTON_NONE, BLUE_NEGATIVE}] = {"None", 0, 80, 120, 120};

  auton_options[{AUTON_LINE, RED_POSITIVE}] = {"Back up", 0, 120, 120, 160};
  auton_options[{AUTON_LINE, RED_NEGATIVE}] = {"Back up", 0, 120, 120, 160};
  auton_options[{AUTON_LINE, BLUE_POSITIVE}] = {"Back up", 0, 120, 120, 160};
  auton_options[{AUTON_LINE, BLUE_NEGATIVE}] = {"Back up", 0, 120, 120, 160};

  auton_options[{AUTON_SKILLS, RED_POSITIVE}] = {"Skills", 360, 80, 480, 160};
  auton_options[{AUTON_SKILLS, RED_NEGATIVE}] = {"Skills", 360, 80, 480, 160};
  auton_options[{AUTON_SKILLS, BLUE_POSITIVE}] = {"Skills", 360, 80, 480, 160};
  auton_options[{AUTON_SKILLS, BLUE_NEGATIVE}] = {"Skills", 360, 80, 480, 160};

  auton_options[{AUTON_BASIC, RED_NEGATIVE}] = {"Basic", 120, 0, 240, 120};
  auton_options[{AUTON_BASIC, BLUE_NEGATIVE}] = {"Basic", 240, 0, 360, 120};
  auton_options[{AUTON_BASIC, RED_POSITIVE}] = {"Basic", 120, 120, 240, 240};
  auton_options[{AUTON_BASIC, BLUE_POSITIVE}] = {"Basic", 240, 120, 360, 240};

  auton_options[{AUTON_AGGRESSIVE, RED_NEGATIVE}] = {"Aggressive", 0, 0, 120, 40};
  auton_options[{AUTON_AGGRESSIVE, BLUE_NEGATIVE}] = {"Aggressive", 360, 0, 480, 40};
  auton_options[{AUTON_AGGRESSIVE, RED_POSITIVE}] = {"Aggressive", 0, 200, 120, 240};
  auton_options[{AUTON_AGGRESSIVE, BLUE_POSITIVE}] = {"Aggressive", 360, 200, 480, 240};

  auton_options[{AUTON_DEFENSIVE, RED_NEGATIVE}] = {"Defensive", 0, 40, 120, 80};
  auton_options[{AUTON_DEFENSIVE, BLUE_NEGATIVE}] = {"Defensive", 360, 40, 480, 80};
  auton_options[{AUTON_DEFENSIVE, RED_POSITIVE}] = {"Defensive", 0, 160, 120, 200};
  auton_options[{AUTON_DEFENSIVE, BLUE_POSITIVE}] = {"Defensive", 360, 160, 480, 200};

  // previous selection
  AutonRoutine prev_auton = AUTON_NONE;
  FieldSide prev_side = RED_POSITIVE;
  lv_obj_t *chosen_auton_box = lv_obj_create(lv_scr_act());
  lv_obj_set_style_radius(chosen_auton_box, 0, 0);
  lv_obj_set_style_pad_all(chosen_auton_box, 0, 0);
  // hide this box by default
  lv_obj_set_size(chosen_auton_box, 0, 0);
  lv_obj_set_pos(chosen_auton_box, 0, 0);
  // transparent background, just a yellow border
  lv_obj_set_style_border_color(chosen_auton_box, lv_color_make(255, 255, 0), 0);
  lv_obj_set_style_border_width(chosen_auton_box, 2, 0);
  lv_obj_set_style_bg_opa(chosen_auton_box, LV_OPA_TRANSP, 0);

  AutonOption chosen_auton_data = auton_options[{selected_auton, selected_field_side}];

  // display selected auton
  lv_obj_set_size(chosen_auton_box, chosen_auton_data.selectionX2 - chosen_auton_data.selectionX1, chosen_auton_data.selectionY2 - chosen_auton_data.selectionY1);
  lv_obj_set_pos(chosen_auton_box, chosen_auton_data.selectionX1, chosen_auton_data.selectionY1);

  do
  {

    //* wait for click
    while (screen::touch_status().touch_status != E_TOUCH_PRESSED)
    {
      delay(20);
    }

    int x = screen::touch_status().x,
        y = screen::touch_status().y;

    // none and skills
    if (threshold(x, 0, 120) && threshold(y, 80, 160))
    {
      if (y < 120)
      {
        selected_auton = AUTON_NONE;
        selected_field_side = RED_POSITIVE;
      }
      else
      {
        selected_auton = AUTON_LINE;
        selected_field_side = RED_POSITIVE;
      }
    }
    else if (threshold(x, 360, 480) && threshold(y, 80, 160))
    {
      selected_auton = AUTON_SKILLS;
      selected_field_side = RED_POSITIVE;
    }
    else
    {

      selected_auton = AUTON_BASIC; // presume unless determined otherwise; lets me set the side once for all 3 options
      if (threshold(x, 0, 240) && threshold(y, 0, 120))
      {
        selected_field_side = RED_NEGATIVE;
      }
      else if (threshold(x, 240, 480) && threshold(y, 0, 120))
      {
        selected_field_side = BLUE_NEGATIVE;
      }
      else if (threshold(x, 0, 240) && threshold(y, 120, 240))
      {
        selected_field_side = RED_POSITIVE;
      }
      else if (threshold(x, 240, 480) && threshold(y, 120, 240))
      {
        selected_field_side = BLUE_POSITIVE;
      }

      // advanced autons
      if (!threshold(x, 120, 360))
      {
        if (threshold(y, 40, 80) || threshold(y, 160, 200))
        {
          selected_auton = AUTON_DEFENSIVE;
        }
        else
        {
          selected_auton = AUTON_AGGRESSIVE;
        }
      }
    }

    if (selected_auton == prev_auton && selected_field_side == prev_side)
    {
      // if the same auton is selected, deselect it
      selected_auton = AUTON_NONE;
      selected_field_side = RED_POSITIVE;
    }
    prev_auton = selected_auton;
    prev_side = selected_field_side;

    AutonOption chosen_auton_data = auton_options[{selected_auton, selected_field_side}];

    // display selected auton
    lv_obj_set_size(chosen_auton_box, chosen_auton_data.selectionX2 - chosen_auton_data.selectionX1, chosen_auton_data.selectionY2 - chosen_auton_data.selectionY1);
    lv_obj_set_pos(chosen_auton_box, chosen_auton_data.selectionX1, chosen_auton_data.selectionY1);

    while (screen::touch_status().touch_status != E_TOUCH_RELEASED)
    {
      delay(20);
    }
  } while (true);

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

void controller_screen()
{
  int error_cycle_ticker = 0;
  int ladybrown_jam_ticker = 0;

  while (true)
  {
    delay(10);

    /*
    example:
    eject:BLU d:red
    ~37C+42C bat100
    1:45 <ERRORS>

    errors:
    DISC_DRIVE
    DISC_IMU
    DISC_EJECT
    DISC_LADYB
    DISC_ROLLR
    DISC_CHAIN

    over_temp
    over_amps

    CHAIN_SNAP

    bad_clamp
    lady_jam

    */

    string line1 = "eject:### d:###";
    string line2 = "~##+##C ##C ##%";
    string line3 = "1:45           ";

    //* LINE 1
    string eject_color_string;
    string detected_color_string;

    switch (ring_color_to_eject)
    {
    case ALLIANCE_RED:
      eject_color_string = "RED";
      break;
    case ALLIANCE_BLUE:
      eject_color_string = "BLU";
      break;
    case ALLIANCE_NEITHER:
      eject_color_string = "___";
      break;
    }

    switch (last_detected_ring_color)
    {
    case ALLIANCE_RED:
      detected_color_string = "red";
      break;
    case ALLIANCE_BLUE:
      detected_color_string = "blu";
      break;
    case ALLIANCE_NEITHER:
      detected_color_string = "___";
      break;
    }

    line1.replace(6, 3, eject_color_string);
    line1.replace(13, 3, detected_color_string);

    //* LINE 2
    // motor temperatures array
    double temps[6] = {
        motor_l1.get_temperature(),
        motor_l2.get_temperature(),
        motor_l3.get_temperature(),
        motor_r1.get_temperature(),
        motor_r2.get_temperature(),
        motor_r3.get_temperature()};

    double avg_temp = 0,
           max_temp = 0;

    for (int i = 0; i < 6; i++)
    {
      avg_temp += temps[i];
      if (temps[i] > max_temp)
        max_temp = temps[i];
    }
    avg_temp /= 6.0;

    // cast to int to truncate decimal
    line2.replace(1, 2, to_string((int)avg_temp));
    line2.replace(4, 2, to_string((int)max_temp));

    // chain temp
    int chain_temp = (int)(chain.get_temperature());
    line2.replace(7, 2, to_string(chain_temp));

    // battery percentage
    int battery_percentage = (int)(battery::get_capacity());
    if (battery_percentage == 100)
      battery_percentage = 99;
    line2.replace(10, 2, to_string(battery_percentage));

    //* LINE 3
    // time
    int minutes = millis() / 60000;
    int seconds = (millis() / 1000) % 60;
    if (minutes > 9) {
      minutes = 9;
      line3.replace(1, 1, "+");
    }
    line3.replace(0, 1, to_string(minutes));
    line3.replace(2, 2, to_string(seconds));

    // errors
    string errors = "";
    // each error will be a consistent 10 characters.
    // for simplicity, this string will basically be an array
    // and if there are multiple errors it'll cycle through by 
    // taking a substring at increasing offsets
    
    if (!(
            motor_l1.is_installed() ||
            motor_l2.is_installed() ||
            motor_l3.is_installed() ||
            motor_r1.is_installed() ||
            motor_r2.is_installed() ||
            motor_r3.is_installed()))
      errors += "DISC_DRIVE";

    if (!imu.is_installed())
      errors += "DISC_IMU  ";

    if (!chain.is_installed())
      errors += "DISC_CHAIN";
    if (!roller.is_installed())
      errors += "DISC_ROLLR";
    if (!ladybrown_motor.is_installed())
      errors += "DISC_LADYB";

    if (!(
            ring_detector.is_installed() ||
            ring_distance.is_installed() ||
            chain_detector.is_installed()))
      errors += "DISC_EJECT";

    // ladybrown jam
    if (
        (abs(ladybrown_motor.get_target_position() - ladybrown_motor.get_position()) > 10) &&
        ladybrown_motor.get_actual_velocity() < 10)
    {
      ladybrown_jam_ticker++;
    }
    else
      ladybrown_jam_ticker = 0;
    if (ladybrown_jam_ticker > 5)
      errors += "lady_jam  ";

    // over temp
    if (
      motor_l1.is_over_temp() ||
      motor_l2.is_over_temp() ||
      motor_l3.is_over_temp() ||
      motor_r1.is_over_temp() ||
      motor_r2.is_over_temp() ||
      motor_r3.is_over_temp() ||
      chain.is_over_temp() ||
      roller.is_over_temp() ||
      ladybrown_motor.is_over_temp()
    ) errors += "over_temp ";

    // over current
    if (
      motor_l1.is_over_current() ||
      motor_l2.is_over_current() ||
      motor_l3.is_over_current() ||
      motor_r1.is_over_current() ||
      motor_r2.is_over_current() ||
      motor_r3.is_over_current() ||
      chain.is_over_current() ||
      roller.is_over_current() ||
      ladybrown_motor.is_over_current()
    ) errors += "over_amps ";

    // broken chain
    if (
      (abs(chain.get_target_velocity()) > 5) &&
      chain.get_efficiency() > 70
    ) errors += "CHAIN_SNAP";


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
  chassis.tank(-30, -30);
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
ASSET(path_rpa3_txt);
ASSET(path_rpa4_txt);
//* Advanced autons
void auton_positive_aggressive(FieldSide side)
{
  // int r = left_or_right(side) == RIGHT ? 1 : -1;

  chassis.setPose({-63, -24, 90}, false);

  // while (true) {
  //   delay(51);
  //   // print in one print statement the x and y pos and rotation of the robot
  //   master.print(0, 0, to_string(chassis.getPose().x).c_str());
  //   delay(51);
  //   master.print(1, 0, to_string(chassis.getPose().y).c_str());
  //   delay(51);
  //   master.print(2, 0, to_string(chassis.getPose().theta).c_str());
  // }

  chassis.follow(path_rpa1_txt, 15, 3000);
  wait();
  roller.move(127);
  delay(500);

  chassis.follow(path_rpa2_txt, 15, 2000, false);
  wait();

  lever.set_value(true);
  intake_direction = 1;

  delay(1000);

  ring_color_to_eject = invert_red_blue(red_or_blue(side));

  chassis.follow(path_rpa3_txt, 15, 4000);
  wait();
  intake_direction = 2;
  delay(2000);

  chassis.moveToPoint(-45, 10, 1200, {.maxSpeed = 50});
  delay(300);
  intake_direction = 1;
  ladybrown_motor.move_absolute(lb_pos_prime + ladybrown_offset, 200);
  wait();

  chassis.turnToPoint(0, -72, 1000);
  chassis.follow(path_rpa4_txt, 15, 4000);

  chassis.waitUntil(5);
  intake_direction = 0;
  chassis.waitUntil(40);
  ladybrown_motor.move_absolute(lb_pos_extend + ladybrown_offset, 200);
  ladybrown_state = 3;

  wait();
}

void auton_positive_defensive(FieldSide side)
{
}

void auton_negative_aggressive(FieldSide side)
{
}

void auton_negative_defensive(FieldSide side, bool mirror = false)
{
  int r = mirror ? 1 : -1;
  r = left_or_right(side) == RIGHT ? 1 : -1;

  ring_color_to_eject = ALLIANCE_NEITHER;

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
  chassis.moveToPoint(r * (24 - 6), 48 + 1, 1500, {.forwards = false, .maxSpeed = 45});
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
  chassis.moveToPoint(r * (4), 43, 3000, {.maxSpeed = 50});
  // if (r == 1)
  // {
  //   chassis.moveToPose(r * (18), (24), 250, 3000);
  //   wait();
  //   hook.set_value(true);
  //   delay(400);
  //   chassis.turnToPoint(r * -24, 48, 500);
  //   wait();
  //   delay(200);
  //   hook.set_value(false);
  //   chassis.moveToPoint(r * 12, 36, 1000);
  //   wait();
  //   delay(800);
  //   chassis.moveToPoint(r * 8, 40, 500);
  // }
  // else
  // {
  // }
}

ASSET(path_skills1_txt);
ASSET(path_skills2_txt);
ASSET(path_skills3_txt);
ASSET(path_skills4_txt);
ASSET(path_skills5_txt);
//* Auton skills
void auton_skills()
{
  ring_color_to_eject = ALLIANCE_NEITHER;

  chassis.setPose({-55, 0, 270}, false);
  // alliance stake
  chassis.moveToPoint(-63, 0, 1000);
  wait();

  ladybrown_motor.move_absolute(-320, 200);
  delay(500);

  // back up
  chassis.moveToPoint(-55, 0, 1000, {.forwards = false});
  wait();

  ladybrown_motor.move_absolute(-10, 200);
  ladybrown_motor.set_brake_mode(MOTOR_BRAKE_COAST);

  chassis.follow(path_skills1_txt, 15, 3000, false);
  wait();
  lever.set_value(true);
  delay(150);

  roller.move(127);
  intake_direction = 1;
  chassis.turnToHeading(180, 1000);
  chassis.follow(path_skills2_txt, 15, 3000);
  chassis.follow(path_skills3_txt, 15, 2000, false);
  chassis.moveToPoint(-50, -48, 1000);
  chassis.turnToPoint(-24, -24, 1200);
  chassis.follow(path_skills4_txt, 15, 10000);
  chassis.turnToHeading(45, 1000);
  chassis.follow(path_skills5_txt, 10, 3000);
  wait();

  chassis.turnToHeading(180, 500);
  chassis.moveToPoint(0, -60, 1000);
}

void old_skills()
{

  chassis.setBrakeMode(MOTOR_BRAKE_BRAKE);

  chassis.setPose({0, 24 - 8 + 0.5, 180}, false);

  ring_color_to_eject = ALLIANCE_NEITHER;

  // while (true) {
  //   delay(51);
  //   // print in one print statement the x and y pos and rotation of the robot
  //   master.print(0, 0, to_string(chassis.getPose().x).c_str());
  //   delay(51);
  //   master.print(1, 0, to_string(chassis.getPose().y).c_str());
  //   delay(51);
  //   master.print(2, 0, to_string(chassis.getPose().theta).c_str());
  // }

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
    chassis.moveToPoint(d * (24 + 3), 24 - 1, 3000, {.forwards = false, .maxSpeed = 80});
    // chassis.moveToPose(24, 24, 270, 2000, {.forwards = false, .maxSpeed = 127});
    wait();

    // delay(150);
    lever.set_value(true);
    delay(350);

    intake_direction = 1;
    roller.move(127);

    // first two rings in a row
    chassis.turnToHeading(90, 1000);
    chassis.moveToPoint(d * (48 + 8), 24, 2000, {.maxSpeed = 80});
    chassis.moveToPoint(d * 48, 24 + 8, 800, {.forwards = false});
    // side ring
    chassis.turnToPoint(d * 48, 12, 2000, {.maxSpeed = 60, .minSpeed = 20, .earlyExitRange = 5});
    chassis.moveToPoint(d * 46, 18, 800);
    wait();
    delay(200);
    chassis.turnToPoint(d * 24, 48, 800);
    chassis.moveToPoint(d * 24, 48, 1500);
    chassis.turnToPoint(d * 48, 48, 1500, {.maxSpeed = 60});
    chassis.moveToPoint(d * 48, 48, 1000);

    // to the wall stake
    chassis.turnToPoint(d * 48 - 7, 72, 1000);
    chassis.moveToPoint(d * 48 - 9, 72 - 2, 2000, {.maxSpeed = 70});
    wait();
    chassis.turnToPoint(d * 72, 72 - 2, 1000);

    ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
    ladybrown_motor.move_absolute(lb_pos_prime + ladybrown_offset, 200);

    chassis.moveToPoint(d * (72 - 10.5 - 4.5), 72 - 2, 1200, {.maxSpeed = 50});
    chassis.turnToHeading(90, 1000);
    wait();
    delay(800);

    // 1st tall wall stake ring
    ladybrown_motor.move_absolute(lb_pos_extend + ladybrown_offset, 200);
    delay(1000);
    chassis.turnToHeading(90 + 30, 800);
    chassis.turnToHeading(90 - 30, 1200);
    chassis.turnToHeading(90, 800);
    chassis.moveToPoint(72, 72, 400);
    wait();

    chassis.moveToPoint(d * (48 - 7), 72, 1000, {.forwards = false});
    ladybrown_motor.move_absolute(lb_pos_stow + ladybrown_offset, 200);
    wait();

    chassis.turnToPoint(d * 48, 72 + 24, 1000);
    chassis.moveToPoint(d * 48, 72 + 24, 1500, {.maxSpeed = 80});
    chassis.turnToPoint(d * 24, 72 + 24, 1000, {.maxSpeed = 60});
    wait();
    delay(500); // let the ring get on the goal

    chassis.moveToPoint(d * 24, 72 + 24, 1000);
    wait();
    chassis.turnToPoint(d * (48 - 7), 72, 1200); // let it be async
    ladybrown_motor.move_absolute(lb_pos_prime + ladybrown_offset, 200);
    chassis.moveToPoint(d * (48 - 7), 72, 1500, {.forwards = false, .maxSpeed = 70});

    delay(1000);

    chassis.turnToPoint(d * 72, 72, 1000);
    chassis.moveToPoint(d * (72 - 10.5 - 3), 72, 1000, {.maxSpeed = 40});
    chassis.turnToHeading(90, 1000);
    wait();

    ladybrown_motor.move_absolute(lb_pos_extend + ladybrown_offset, 200);
    delay(1000);
    chassis.turnToHeading(90 + 30, 800);
    chassis.turnToHeading(90 - 30, 1200);
    chassis.turnToHeading(90, 800);
    chassis.moveToPoint(72, 72, 400);
    wait();

    chassis.moveToPoint(d * (48 - 7), 72, 1000, {.forwards = false});
    wait();
    ladybrown_motor.move_absolute(lb_pos_stow + ladybrown_offset, 200);

    // put the goal in the corner lil bro
    chassis.moveToPoint(d * 60, 18, 2000, {.forwards = false});
    wait();

    lever.set_value(false);

    // break;
    if (d == 1)
    {
      d = -1;
      continue;
    }
    else
      break;
  }
}

void skills_very_old()
{
  ring_color_to_eject = ALLIANCE_NEITHER;
  chassis.setPose({18 + 1, 15 + 3, 210});

  chassis.moveToPoint(24, 24, 1000, {.forwards = false, .maxSpeed = 50});
  chassis.waitUntilDone();

  delay(300);
  lever.set_value(true);

  intake_direction = 1;

  roller.move(127);
  delay(300);

  // testing coords
  // chassis.turnToPoint(0, 24, 1000);
  // chassis.moveToPoint(0, 24, 1000);
  // return;

  // close to ladder ring
  chassis.turnToPoint(24, 48, 1000);
  chassis.waitUntilDone();
  delay(300);
  chassis.moveToPoint(24, 48, 1500);
  chassis.waitUntilDone();
  delay(300);

  chassis.turnToPoint(48, 48, 1000);
  chassis.waitUntilDone();
  delay(300);
  chassis.moveToPoint(48, 48, 1500);
  chassis.waitUntilDone();
  delay(300);

  // right side of the ring triangle
  chassis.turnToPoint(48, 24 - 24, 1500);
  chassis.waitUntilDone();
  delay(300);
  chassis.moveToPoint(48, 24, 1500);
  chassis.waitUntilDone();
  delay(1000);

  chassis.turnToPoint(60, 28, 1500);
  chassis.waitUntilDone();
  delay(300);
  chassis.moveToPoint(60 - 4, 24, 2000);
  chassis.waitUntilDone();
  delay(1000);

  // go drop the thing in the corner
  chassis.moveToPoint(30, 30, 2000, {.maxSpeed = 60});
  chassis.moveToPoint(60 - 5, 12 + 5, 2000, {.forwards = false});
  chassis.waitUntilDone();

  delay(100);

  intake_direction = 0;
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
  delay(150);
  lever.set_value(true);
  delay(150);
  intake_direction = 1;

  // ! MIRROR POINT START

  // close to ladder ring
  chassis.turnToPoint(-24, 48, 2000);
  chassis.waitUntilDone();
  delay(300);
  chassis.moveToPoint(-24, 48, 2000);
  chassis.waitUntilDone();
  delay(300);

  // ! extra piece

  // ! end extra piece

  chassis.turnToPoint(-48, 48, 1000);
  chassis.waitUntilDone();
  delay(300);
  chassis.moveToPoint(-48, 48, 2000);
  chassis.waitUntilDone();
  delay(300);

  // right side of the ring triangle
  chassis.turnToPoint(-48, 24 - 24, 2000);
  chassis.waitUntilDone();
  delay(300);
  chassis.moveToPoint(-48, 24 - 6, 2000);
  chassis.waitUntilDone();
  delay(1000);

  chassis.turnToPoint(-60, 24, 2000);
  chassis.waitUntilDone();
  delay(500);
  chassis.moveToPoint(-60 + 4, 24 + 7, 2000);
  chassis.waitUntilDone();
  delay(1000);

  // go drop the thing in the corner
  chassis.moveToPoint(-30, 30, 2000);
  chassis.moveToPoint(-60 - 5, 12 - 5, 2000, {.forwards = false});
  chassis.waitUntilDone();

  // delay(1000);

  intake_direction = 0;
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
  // chassis.turnToPoint(-24, 72 + 60, 2000, {.forwards = false});
  // chassis.moveToPoint(-24, 72 + 60, 2000, {.forwards = false, .maxSpeed = 40});

  // chassis.waitUntilDone();

  // delay(200);
  // lever.set_value(true);
  // delay(200);

  // chassis.turnToPoint(-72, 72 + 72, 1000);
  chassis.turnToPoint(-72 + 9, 72 + 72 - 5, 1200);
  chassis.moveToPoint(-72 + 9, 72 + 72 - 5, 3000);

  // chassis.turnToPoint(-72, 72 + 72 - 48, 1200, {.forwards = false});
  // chassis.moveToPoint(-72 + 5, 72 + 72 - 5, 3000, {.forwards = false});
  chassis.waitUntilDone();
  delay(200);
  chassis.moveToPoint(48, 72 + 72 - 10, 2000, {.forwards = false});
  // lever.set_value(false);
  // intake_direction = 1;
  chassis.moveToPoint(0 + 10, 72 + 72, 3000);
  chassis.moveToPoint(72 - 9, 72 + 72 - 5, 3000);

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
  // unjammer.remove();
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
  timer_offset = millis();

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
  timer_offset = millis();
  // chassis is already calibrated in initialize()

  left_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  right_motors.set_brake_mode(MOTOR_BRAKE_BRAKE);
  auton_selector_active = false;

  //! HARDCODER
  // selected_auton =
  // selected_auton = AUTON_BASIC;
  // selected_auton = AUTON_AGGRESSIVE;
  // selected_auton = AUTON_LINE;

  // selected_auton = AUTON_DEFENSIVE;

  // selected_field_side = RED_POSITIVE;
  // selected_field_side = RED_NEGATIVE;
  // selected_field_side = BLUE_POSITIVE;

  // selected_field_side = BLUE_NEGATIVE;

  // selected_auton = AUTON_SKILLS;

  ring_color_to_eject = invert_red_blue(red_or_blue(selected_field_side));
  ring_color_to_eject = ALLIANCE_NEITHER;

  // skills_very_old();
  // return;

  switch (selected_auton)
  {
  //* symmetrical autons
  case AUTON_NONE:
    break;

  case AUTON_LINE:
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
    ring_color_to_eject = ALLIANCE_NEITHER;
    // auton_skills();
    old_skills();
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
  timer_offset = millis();

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
      ladybrown_motor.move_absolute(lb_pos_extend + ladybrown_offset, 200);

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
