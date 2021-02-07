cmake_minimum_required (VERSION 2.6)

project (iTekon)

add_executable(iTekon-test iTekon-test.c)

ADD_SUBDIRECTORY(lib)

link_libraries("./lib/libusb-1.0.a")
target_link_libraries(iTekon-test iTekon_usb_static)

INSTALL (TARGETS iTekon-test RUNTIME  DESTINATION bin/iTekon-usb)

