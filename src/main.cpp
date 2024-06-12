#include "main.h"
// #include "lemlib/api.hpp"
#include "pros/misc.h"
#include "pros/misc.hpp"

#include <algorithm>
#include <math.h>
#include <string.h>

#include "devices.h"

using namespace std;

void controller_screen() {

}

void initialize() {
	// settings file
	FILE *out = fopen("sout", "w");
	fputs("Opening settings.txt...", out);
	FILE *fp = fopen("/settings.txt", "r");
	if (fp == NULL) {
		fputs("settings.txt not found, creating...", out);
		fp = fopen("/settings.txt", "w");
		fprintf(fp, "");
		fclose(fp);
	} else fputs("settings.txt found", out);

	// controller task
	Task controller_task(controller_screen);
	fputs("Starting controller screen task", out);
}

void disabled() {}

void competition_initialize() {}

void autonomous() {}

void opcontrol() {
  Controller master(E_CONTROLLER_MASTER);
  MotorGroup left_mg({-18, -19, 20});
  MotorGroup right_mg({8, 9, -10});

  int turning_speed = 50;

  int interval = 0;

  while (true) {
    interval++;

    if (master.get_digital_new_press(E_CONTROLLER_DIGITAL_UP))
      turning_speed += turning_speed >= 100 ? 0 : 10;
    if (master.get_digital_new_press(E_CONTROLLER_DIGITAL_DOWN))
      turning_speed -= turning_speed <= 0 ? 0 : 10;

    if (interval == 20) {
      master.set_text(0, 0, to_string(turning_speed).c_str());
	  interval = 0;
    }

    double forward = master.get_analog(E_CONTROLLER_ANALOG_LEFT_Y),
           turn = master.get_analog(E_CONTROLLER_ANALOG_RIGHT_X) *
                  (0.01 * turning_speed);

    double left = forward + turn, right = forward - turn;
    double max_val = max(127.0, max(abs(left), abs(right)));
    left /= max_val;
    right /= max_val;

    left_mg.move(left * 127);
    right_mg.move(right * 127);

    delay(20);
  }
}