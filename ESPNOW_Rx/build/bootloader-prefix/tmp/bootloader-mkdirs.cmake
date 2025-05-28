# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/matt/esp/v5.4.1/esp-idf/components/bootloader/subproject"
  "/home/matt/ESP Projects/ESPNOW_Project/ESPNOW_Rx/build/bootloader"
  "/home/matt/ESP Projects/ESPNOW_Project/ESPNOW_Rx/build/bootloader-prefix"
  "/home/matt/ESP Projects/ESPNOW_Project/ESPNOW_Rx/build/bootloader-prefix/tmp"
  "/home/matt/ESP Projects/ESPNOW_Project/ESPNOW_Rx/build/bootloader-prefix/src/bootloader-stamp"
  "/home/matt/ESP Projects/ESPNOW_Project/ESPNOW_Rx/build/bootloader-prefix/src"
  "/home/matt/ESP Projects/ESPNOW_Project/ESPNOW_Rx/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/matt/ESP Projects/ESPNOW_Project/ESPNOW_Rx/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/matt/ESP Projects/ESPNOW_Project/ESPNOW_Rx/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
