#include "main.h"
#include "lemlib/api.hpp"
#include "pros/abstract_motor.hpp"
#include "pros/misc.h"
#include "pros/motors.h"

Controller master(E_CONTROLLER_MASTER);
Controller partner(E_CONTROLLER_PARTNER);

Motor motor_l1(-18, MotorGearset::blue, MotorUnits::degrees),
    motor_l2(-19, MotorGearset::blue, MotorUnits::degrees),
    motor_l3(20, MotorGearset::blue, MotorUnits::degrees),
    motor_r1(8, MotorGearset::blue, MotorUnits::degrees),
    motor_r2(9, MotorGearset::blue, MotorUnits::degrees),
    motor_r3(-10, MotorGearset::blue, MotorUnits::degrees);

