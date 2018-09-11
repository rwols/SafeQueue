if (NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
    message(FATAL_ERROR "Please clone the git submodules.")
endif()

set(BUILD_SHARED_LIBS ON CACHE BOOL "")
set(INSTALL_GTEST OFF CACHE INTERNAL "")
set(BUILD_GMOCK OFF CACHE INTERNAL "")
add_subdirectory(googletest)
add_executable(Test${PROJECT_NAME} ${PROJECT_NAME}.cpp)
set_target_properties(Test${PROJECT_NAME}
    PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} gtest_main)
include(GoogleTest)
gtest_discover_tests(Test${PROJECT_NAME} PROPERTIES TIMEOUT 10)
