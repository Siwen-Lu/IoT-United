#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	src/mesh_light_handler.c
	src/button_func.c
	src/speaker_func.c
	src/model_handler.c
	src/mesh_cmd_handler.c
	src/rssi_thread.c
	include/rssi_thread.h
	src/cmd_thread.c
	include/cmd_thread.h
	include/button_func.h
	include/mesh_cmd_handler.h
	include/mesh_light_handler.h
	include/model_handler.h
	include/speaker_func.h)
target_include_directories(app PRIVATE include)
# NORDIC SDK APP END
