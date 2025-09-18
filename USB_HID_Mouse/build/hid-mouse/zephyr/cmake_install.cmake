# Install script for directory: C:/ncs/v3.1.0/zephyr

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Zephyr-Kernel")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/arch/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/lib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/soc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/boards/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/subsys/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/drivers/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/nrf/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/hostap/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/mcuboot/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/mbedtls/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/trusted-firmware-m/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/cjson/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/azure-sdk-for-c/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/cirrus-logic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/openthread/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/memfault-firmware-sdk/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/canopennode/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/chre/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/lz4/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/nanopb/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/zscilib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/cmsis/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/cmsis-dsp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/cmsis-nn/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/cmsis_6/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/fatfs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/hal_nordic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/hal_st/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/hal_tdk/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/hal_wurthelektronik/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/liblc3/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/libmetal/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/littlefs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/loramac-node/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/lvgl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/mipi-sys-t/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/nrf_wifi/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/open-amp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/percepio/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/picolibc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/segger/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/tinycrypt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/uoscore-uedhoc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/zcbor/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/nrfxlib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/nrf_hw_models/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/modules/connectedhomeip/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/kernel/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/cmake/flash/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/cmake/usage/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/ncs/v3.1.0/zephyr/samples/subsys/usb/hid-mouse/build/hid-mouse/zephyr/cmake/reports/cmake_install.cmake")
endif()

