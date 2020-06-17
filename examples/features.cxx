#include <cppshell.hpp>

int main(int argc, char **argv)
{
    using namespace cppshell;

    std::string ls;

    // ls -la > test_file
    "ls -la"_e > "test_file";

    // cat test_file
    "cat test_file"_e;

    // rm test_file
    "rm test_file"_e;

    if (!"false"_e) {
        std::cout << "command returned error\n";
    }
}
