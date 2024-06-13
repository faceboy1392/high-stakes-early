#include "main.h"
#include "lemlib/api.hpp"

extern Controller master;
extern Controller partner;

extern Motor motor_l1,
    motor_l2,
    motor_l3,
    motor_r1,
    motor_r2,
    motor_r3;

extern MotorGroup left_motors,
    right_motors;

extern Imu imu;

extern lemlib::Drivetrain drivetrain;
extern lemlib::OdomSensors sensors;
extern lemlib::ControllerSettings lateral_controller;
extern lemlib::ControllerSettings angular_controller;
extern lemlib::Chassis chassis;