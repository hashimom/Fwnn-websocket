ADD_EXECUTABLE(izumo-fwnn 
  main.c 
  izumowebsock.c)

TARGET_LINK_LIBRARIES(izumo-fwnn cwebsocket)

INSTALL(TARGETS izumo-fwnn DESTINATION bin)

