cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(MiddleStation-Zephyr)
FILE(GLOB app_sources core/*.c*)
target_sources(app PRIVATE ${app_sources})
# NORDIC SDK APP END
zephyr_library_include_directories(.)