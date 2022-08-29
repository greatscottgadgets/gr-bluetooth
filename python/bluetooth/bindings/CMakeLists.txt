# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT bluetooth_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT bluetooth_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND bluetooth_python_files
    multi_block_python.cc
    multi_hopper_python.cc
    multi_LAP_python.cc
    multi_sniffer_python.cc
    multi_UAP_python.cc
    packet_python.cc
    piconet_python.cc
    python_bindings.cc)

GR_PYBIND_MAKE_OOT(bluetooth
   ../../..
   gr::bluetooth
   "${bluetooth_python_files}")

# copy in bindings .so file for use in QA test module
add_custom_target(
  copy_bindings_for_tests ALL
  COMMAND
    ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/*.so"
    ${CMAKE_BINARY_DIR}/test_modules/gnuradio/bluetooth/
  DEPENDS bluetooth_python)

install(TARGETS bluetooth_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/bluetooth COMPONENT pythonapi)
