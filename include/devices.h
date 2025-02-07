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

extern Motor chain;
extern Motor roller;

extern Motor ladybrown_motor;
extern Rotation ladybrown_sensor;
extern adi::DigitalIn ladybrown_limit;

extern Optical ring_detector;
extern Distance ring_distance;
extern Distance chain_detector;

extern Imu imu;

extern adi::DigitalOut lever;
extern adi::DigitalOut hook;

extern lemlib::Drivetrain drivetrain;
extern lemlib::OdomSensors sensors;
extern lemlib::ControllerSettings lateral_controller;
extern lemlib::ControllerSettings angular_controller;
extern lemlib::Chassis chassis;