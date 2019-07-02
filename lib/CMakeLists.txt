SET (LIBITEKONUSB_SRC iTekon-usb.c rbuf.c sm4.c)

ADD_LIBRARY (iTekon_usb_static STATIC ${LIBITEKONUSB_SRC})

SET_TARGET_PROPERTIES (iTekon_usb_static PROPERTIES OUTPUT_NAME "iTekon-usb")

SET_TARGET_PROPERTIES (iTekon_usb_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

INSTALL (TARGETS iTekon_usb_static LIBRARY DESTINATION lib/iTekon-usb ARCHIVE DESTINATION lib/iTekon-usb)
INSTALL (FILES iTekon-usb.h DESTINATION include/iTekon-usb)

#link_libraries("/root/iTekon-usb-master/lib/libusb-1.0.a")
target_link_libraries(iTekon_usb_static ${CMAKE_CURRENT_SOURCE_DIR}/libusb-1.0.a pthread )
# "-Wl,--whole-archive"
#"-Wl,--no-whole-archive")

