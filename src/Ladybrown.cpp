#include "devices.h"
#include "main.h"

class Ladybrown
{
private:
  int ladybrown_state = 0;

public:
  void run() {
    double lb_motor_pos = ladybrown_motor.get_position();
    double lb_sensor_pos = ladybrown_sensor.get_angle();
    double lb_offset = -lb_sensor_pos * 5 - lb_motor_pos;
    if (ladybrown_state == 0)
    {
      // if (ladybrown_sensor.get_position() > 0)
      // ladybrown.move(ladybrown_pid.update(-100, lb_pos));
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_COAST);
      if (ladybrown_motor.get_position() >= 20)
        ladybrown_motor.move_absolute(5 + lb_offset, 200);
    }
    else if (ladybrown_state == 1)
    {
      // ladybrown.move(ladybrown_pid.update(-95, lb_pos));
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
      ladybrown_motor.move_absolute(-15 + lb_offset, 200);
    }
    else if (ladybrown_state == 2)
    {
      // ladybrown.move(ladybrown_pid.update(0, lb_pos));
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
      ladybrown_motor.move_absolute(-620 + lb_offset, 200);
    }
    else if (ladybrown_state == 3)
    {
      // ladybrown.move(ladybrown_pid.update(70, lb_pos));
      ladybrown_motor.set_brake_mode(MOTOR_BRAKE_HOLD);
      ladybrown_motor.move_absolute(-920 + lb_offset, 200);
    }
  }

  void setState(int state)
  {
    ladybrown_state = state;
  }
  int getState()
  {
    return ladybrown_state;
  }
};
