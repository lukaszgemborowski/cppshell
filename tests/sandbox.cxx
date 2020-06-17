#include "catch.hpp"
#include <cppshell.hpp>

TEST_CASE("sandbox", "[sandbox]")
{
    using namespace cppshell;

    "g++ --version"_e;
    "ls -la"_e | "grep cxx"_e > "./output";
}
