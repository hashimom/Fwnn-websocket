ADD_EXECUTABLE(ws-wnn 
  main.c 
  izumowebsock.c)

TARGET_LINK_LIBRARIES(ws-wnn cwebsocket)
TARGET_LINK_LIBRARIES(ws-wnn easyfwnn)
TARGET_LINK_LIBRARIES(ws-wnn aoi)

INSTALL(TARGETS ws-wnn DESTINATION bin)

