cmake_minimum_required(VERSION 3.16.0)

# To make sure PIO libraries downloded in the .ini with lib_deps are accessible to espidf
get_filename_component(configName "${CMAKE_BINARY_DIR}" NAME)
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/.pio/libdeps/${configName}/esp_littlefs")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(EProject)
