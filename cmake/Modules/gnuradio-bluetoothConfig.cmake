find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_BLUETOOTH gnuradio-bluetooth)

FIND_PATH(
    GR_BLUETOOTH_INCLUDE_DIRS
    NAMES gnuradio/bluetooth/api.h
    HINTS $ENV{BLUETOOTH_DIR}/include
        ${PC_BLUETOOTH_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_BLUETOOTH_LIBRARIES
    NAMES gnuradio-bluetooth
    HINTS $ENV{BLUETOOTH_DIR}/lib
        ${PC_BLUETOOTH_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-bluetoothTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_BLUETOOTH DEFAULT_MSG GR_BLUETOOTH_LIBRARIES GR_BLUETOOTH_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_BLUETOOTH_LIBRARIES GR_BLUETOOTH_INCLUDE_DIRS)
