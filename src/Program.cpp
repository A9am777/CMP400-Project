#include "Window/HaboobWindow.h"

#include <args.hxx>

using namespace Haboob;
int main(int argc, char* argv[])
{
  args::ArgumentParser parser("This is a test program.", "This goes after the options.");

  args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
  args::Group group(parser, "This group is all exclusive:", args::Group::Validators::DontCare);
  args::Flag foo(group, "foo", "The foo flag", { 'f', "foo" });
  args::CompletionFlag completion(parser, { "complete" });
  //args::ValueFlag<float> f;

  Environment env(&parser);
  HaboobWindow app;
  app.setupEnv(&env);
  env.setupEnvironment();

  try
  {
    parser.ParseCLI(argc, argv);
  }
  catch (args::Completion& e)
  {
    std::cout << e.what();
    return 0;
  }
  catch (args::Help&)
  {
    std::cout << parser;
    return 0;
  }
  catch (args::ParseError& e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  app.appContext(L"HaboobCMP400");
  app.create(L"Haboob", CW_USEDEFAULT, CW_USEDEFAULT);
  app.show();
  app.run();

  return 0;
}