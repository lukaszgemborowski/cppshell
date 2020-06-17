set(
    include-dir
    ${project-root}/include/
)

find_package(
    Boost 1.70.0 REQUIRED
)

find_package(Threads REQUIRED)

add_library(
    cppshell
    STATIC
    dummy.cxx
)

target_link_libraries(
    cppshell
    PUBLIC
    boost_filesystem
    boost_system
)

target_compile_features(
    cppshell
    PUBLIC
    cxx_std_17
)

target_include_directories(
    cppshell
    PUBLIC
    ${include-dir}
)
