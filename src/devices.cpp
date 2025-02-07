#include "main.h"
#include "lemlib/api.hpp"
#include "pros/adi.h"
#include "pros/adi.hpp"
#include "pros/motors.h"

Controller master(E_CONTROLLER_MASTER);
Controller partner(E_CONTROLLER_PARTNER);

signed char motor_l1_port = -15,
motor_l2_port = -16,
motor_l3_port = 17,
motor_r1_port = 18,
motor_r2_port = 19,
motor_r3_port = -20;

Motor motor_l1(motor_l1_port, MotorGearset::blue, MotorUnits::degrees),
    motor_l2(motor_l2_port, MotorGearset::blue, MotorUnits::degrees),
    motor_l3(motor_l3_port, MotorGearset::blue, MotorUnits::degrees),
    motor_r1(motor_r1_port, MotorGearset::blue, MotorUnits::degrees),
    motor_r2(motor_r2_port, MotorGearset::blue, MotorUnits::degrees),
    motor_r3(motor_r3_port, MotorGearset::blue, MotorUnits::degrees);

MotorGroup left_motors({motor_l1_port, motor_l2_port, motor_l3_port}, MotorGearset::blue, MotorUnits::degrees),
    right_motors({motor_r1_port, motor_r2_port, motor_r3_port}, MotorGearset::blue, MotorUnits::degrees);

Motor chain(11, MotorGears::blue, MotorUnits::degrees);
Motor roller(5);

Motor ladybrown_motor(1, MotorGears::green, MotorUnits::degrees);
Rotation ladybrown_sensor(2);
adi::DigitalIn ladybrown_limit('H');

Optical ring_detector(6);
Distance ring_distance(4);
Distance chain_detector(7);

Imu imu(14);

// adi::DigitalOut rollers('A');
adi::DigitalOut lever('A');
adi::DigitalOut hook('B');

lemlib::Drivetrain drivetrain(
    &left_motors,
    &right_motors,
    13,
    lemlib::Omniwheel::NEW_325,
    360,
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
                                              5, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in inches
                                              200, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              20 // maximum acceleration (slew)
);

// angular PID controller
lemlib::ControllerSettings angular_controller(1.8, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              11.5, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in degrees
                                              200, // small error range timeout, in milliseconds
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