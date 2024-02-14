#include <catch2/catch_test_macros.hpp>

unsigned int Factorial(unsigned int number) {
  return number <= 1 ? number : Factorial(number - 1) * number;
}

TEST_CASE("Example", "[ex]") {
  REQUIRE(1 == 1);
  REQUIRE(2 == 2);
  REQUIRE(0x06 == 6);
  REQUIRE(1 << 2 == 3);
}