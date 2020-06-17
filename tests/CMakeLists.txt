add_executable(
    tests
    main.cxx
    sandbox.cxx
)

target_link_libraries(
    tests
    PRIVATE
    cppshell
)

target_include_directories(
    tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
