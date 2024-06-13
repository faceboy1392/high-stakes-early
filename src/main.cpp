#include "main.h"

#include <algorithm>
#include <math.h>
#include <string.h>

#include "devices.h"

using namespace std;

void controller_screen() {
	while (true) {
		master.clear();
		master.set_text(0, 0, "Hello, world!");
		delay(200);
	}
}

void brain_screen() {
	while (true) {
		screen::print(TEXT_LARGE, 0, "Hello, world!");
		delay(80);
	}
}

void initialize() {
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
	} else fputs("settings.txt found", out);

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
	}else {
		chassis.curvature(forward, turn);
	}



    delay(20);
  }
}