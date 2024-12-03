#include "main.h"
#include "lemlib/api.hpp"
#include "pros/adi.h"
#include "pros/adi.hpp"

Controller master(E_CONTROLLER_MASTER);
Controller partner(E_CONTROLLER_PARTNER);

signed char motor_l1_port = -8,
motor_l2_port = -10,
motor_l3_port = 9,
motor_r1_port = 18,
motor_r2_port = 20,
motor_r3_port = -19;

Motor motor_l1(motor_l1_port, MotorGearset::blue, MotorUnits::degrees),
    motor_l2(motor_l2_port, MotorGearset::blue, MotorUnits::degrees),
    motor_l3(motor_l3_port, MotorGearset::blue, MotorUnits::degrees),
    motor_r1(motor_r1_port, MotorGearset::blue, MotorUnits::degrees),
    motor_r2(motor_r2_port, MotorGearset::blue, MotorUnits::degrees),
    motor_r3(motor_r3_port, MotorGearset::blue, MotorUnits::degrees);

MotorGroup left_motors({motor_l1_port, motor_l2_port, motor_l3_port}, MotorGearset::blue, MotorUnits::degrees),
    right_motors({motor_r1_port, motor_r2_port, motor_r3_port}, MotorGearset::blue, MotorUnits::degrees);

MotorGroup intake({17, -7}, MotorGearset::green, MotorUnits::degrees);

Imu imu(2);

adi::DigitalOut rollers('A');
adi::DigitalOut lever('B');
adi::DigitalOut hook('C');

lemlib::Drivetrain drivetrain(
    &left_motors,
    &right_motors,
    11,
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
lemlib::ControllerSettings lateral_controller(12, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              3.3, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in inches
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              20 // maximum acceleration (slew)
);

// angular PID controller
lemlib::ControllerSettings angular_controller(2.35, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              7, // derivative gain (kD)
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