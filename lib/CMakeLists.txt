# Copyright 2011 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

add_library(gr-bluetooth SHARED
    gr_bluetooth_multi_block.cc
    gr_bluetooth_multi_LAP.cc
    gr_bluetooth_multi_UAP.cc
    gr_bluetooth_piconet.cc
    gr_bluetooth_multi_hopper.cc
    gr_bluetooth_multi_sniffer.cc
    gr_bluetooth_packet.cc
    tun.cc
)

target_link_libraries(gr-bluetooth ${Boost_LIBRARIES} ${GRUEL_LIBRARIES} ${GNURADIO_CORE_LIBRARIES})
set_target_properties(gr-bluetooth PROPERTIES DEFINE_SYMBOL "gr_bluetooth_EXPORTS")

########################################################################
# Install built library files
########################################################################
install(TARGETS gr-bluetooth
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)
