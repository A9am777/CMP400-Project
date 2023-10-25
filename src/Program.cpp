#include "Window/HaboobWindow.h"

using namespace Haboob;
int main(int argc, char* argv[])
{
  HaboobWindow app;
  app.appContext(L"HaboobCMP400");
  app.create(L"Haboob", CW_USEDEFAULT, CW_USEDEFAULT);
  app.show();
  app.run();

  // TODO: add stuff here
  return 0;
}