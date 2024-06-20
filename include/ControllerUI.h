#include "main.h"
#include "devices.h"

#include <string.h>
#include <stack>

using namespace std;

class ControllerPage {
public:
  virtual ~ControllerPage();
  virtual void display();
  virtual void handleInput();
};

class ControllerUI {
private:
  stack<ControllerPage *> pages;

public:
  ControllerUI();

  void pushPage(ControllerPage *page);
  void popPage();
  void displayCurrentPage();
  void handleInput();
};