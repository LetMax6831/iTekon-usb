cmake_minimum_required (VERSION 2.6)

project (iTekon)

add_executable(iTekon-test iTekon-test.c)

ADD_SUBDIRECTORY(lib)

target_link_libraries(iTekon-test iTekon_usb_static)

INSTALL (TARGETS iTekon-test RUNTIME  DESTINATION bin/iTekon_usb)

#cmake -DCMAKE_INSTALL_PREFIX=/home/grool/10__target/iTekon
