#include "devices.h"
#include "main.h"

#include <stack>
#include <string.h>

using namespace std;

class ControllerPage {
public:
  virtual ~ControllerPage(){};
  virtual void display();
  virtual void handleInput();
};

class ControllerUI {
private:
  stack<ControllerPage *> pages;

public:
  ControllerUI() {}

  void pushPage(ControllerPage *page) {}
  void popPage() {}
  void displayCurrentPage() {}
  void handleInput() {}
};

// void printToController(string line1, string line2, string line3) {
//   master.print(0, 0, line1.c_str());
//   delay(50);
//   master.print(1, 0, line2.c_str());
//   delay(50);
//   master.print(2, 0, line3.c_str());
//   delay(50);
// }

void controller_task() {}