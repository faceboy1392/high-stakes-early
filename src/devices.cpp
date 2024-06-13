#include "main.h"
#include "lemlib/api.hpp"

Controller master(E_CONTROLLER_MASTER);
Controller partner(E_CONTROLLER_PARTNER);

Motor motor_l1(-18, MotorGearset::blue, MotorUnits::degrees),
    motor_l2(-19, MotorGearset::blue, MotorUnits::degrees),
    motor_l3(20, MotorGearset::blue, MotorUnits::degrees),
    motor_r1(8, MotorGearset::blue, MotorUnits::degrees),
    motor_r2(9, MotorGearset::blue, MotorUnits::degrees),
    motor_r3(-10, MotorGearset::blue, MotorUnits::degrees);

MotorGroup left_motors({-18, -19, 20}, MotorGearset::blue, MotorUnits::degrees),
    right_motors({8, 9, -10}, MotorGearset::blue, MotorUnits::degrees);

Imu imu(6);

lemlib::Drivetrain drivetrain(
    &left_motors,
    &right_motors,
    15.5,
    lemlib::Omniwheel::NEW_4,
    257.143,
    2
);

lemlib::OdomSensors sensors(
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &imu
);

// lateral PID controller
lemlib::ControllerSettings lateral_controller(10, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              3, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in inches
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              20 // maximum acceleration (slew)
);

// angular PID controller
lemlib::ControllerSettings angular_controller(2, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              10, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in degrees
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in degrees
                                              500, // large error range timeout, in milliseconds
                                              0 // maximum acceleration (slew)
);

lemlib::Chassis chassis(
    drivetrain,
    lateral_controller,
    angular_controller,
    sensors
);