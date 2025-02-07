#include "devices.h"
#include "main.h"

class Ladybrown
{
private:
  int ladybrown_state;

public:
  void run();

  void setState(int state);
  int getState();
};