#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/main.h
  src/ble_comms.h
  src/conn_queue.h
  src/conn_queue.c
  src/ble_comms.c
  src/device_record.h
  src/device_record.c)
# NORDIC SDK APP END
zephyr_library_include_directories(.)
