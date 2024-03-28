# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Yobboy/esp/v5.2/esp-idf/components/bootloader/subproject"
  "D:/Code/2024-3/tusb_hid/build/bootloader"
  "D:/Code/2024-3/tusb_hid/build/bootloader-prefix"
  "D:/Code/2024-3/tusb_hid/build/bootloader-prefix/tmp"
  "D:/Code/2024-3/tusb_hid/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Code/2024-3/tusb_hid/build/bootloader-prefix/src"
  "D:/Code/2024-3/tusb_hid/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Code/2024-3/tusb_hid/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Code/2024-3/tusb_hid/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
