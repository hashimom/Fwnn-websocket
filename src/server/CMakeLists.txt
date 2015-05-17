ADD_EXECUTABLE(izumo-fwnn 
  main.c 
  izumowebsock.c)

TARGET_LINK_LIBRARIES(izumo-fwnn manyowebsock)
TARGET_LINK_LIBRARIES(izumo-fwnn easyfwnn)

INSTALL(TARGETS izumo-fwnn DESTINATION bin)

