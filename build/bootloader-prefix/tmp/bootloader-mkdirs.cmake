# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/hualde/esp/v5.3.2/esp-idf/components/bootloader/subproject"
  "/home/hualde/misCosas/06964700v2/esp32_webserver_lang/build/bootloader"
  "/home/hualde/misCosas/06964700v2/esp32_webserver_lang/build/bootloader-prefix"
  "/home/hualde/misCosas/06964700v2/esp32_webserver_lang/build/bootloader-prefix/tmp"
  "/home/hualde/misCosas/06964700v2/esp32_webserver_lang/build/bootloader-prefix/src/bootloader-stamp"
  "/home/hualde/misCosas/06964700v2/esp32_webserver_lang/build/bootloader-prefix/src"
  "/home/hualde/misCosas/06964700v2/esp32_webserver_lang/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/hualde/misCosas/06964700v2/esp32_webserver_lang/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/hualde/misCosas/06964700v2/esp32_webserver_lang/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
