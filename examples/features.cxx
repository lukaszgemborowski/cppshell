#include <cppshell.hpp>

int main(int argc, char **argv)
{
    using namespace cppshell;

    "g++ --version"_e;
    "ls -la"_e | "grep cxx"_e > "./output";
}
