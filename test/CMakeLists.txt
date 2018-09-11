if (NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
    message(FATAL_ERROR "Please clone the git submodules.")
endif()

set(BUILD_SHARED_LIBS ON)
set(INSTALL_GTEST OFF)
add_subdirectory(googletest)
add_executable(Test${PROJECT_NAME} ${PROJECT_NAME}.cpp)
set_target_properties(Test${PROJECT_NAME}
    PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} gmock gmock_main)
include(GoogleTest)
gtest_discover_tests(Test${PROJECT_NAME} PROPERTIES TIMEOUT 1)
