#include <cppshell.hpp>

int main(int argc, char **argv)
{
    using namespace cppshell;

    if ("g++ --version"_e) {
        std::cout << "g++ succesfully executed\n";
    }

    "ls -la"_e | "grep cxx"_e > "./output";
}
