cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(CMAKE_MACOSX_RPATH 1)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

project(SafeQueue VERSION 0.1.0 LANGUAGES CXX)
option(${PROJECT_NAME}_BUILD_TESTS "Build and run unit tests" ON)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_14)

# Unit tests use googletest as a git-submodule.
if(SafeQueue_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Enable find_package(SafeQueue) from other projects.
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(namespace rwols::)
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
write_basic_package_version_file(
    "${version_config}"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
    Config.cmake.in
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}")

# Install targets, include dirs, and cmake package files.
install(TARGETS ${PROJECT_NAME}
        EXPORT SafeQueueTargets
        PUBLIC_HEADER DESTINATION include)
install(DIRECTORY include DESTINATION . FILES_MATCHING PATTERN *.hpp)
install(FILES "${project_config}" "${version_config}"
        DESTINATION "${config_install_dir}")
install(EXPORT SafeQueueTargets NAMESPACE ${namespace}
        DESTINATION "${config_install_dir}")
