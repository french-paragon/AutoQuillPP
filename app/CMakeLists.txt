set(PROG_NAME "${PROJECT_NAME}")

set(PROG_SRC
    main.cpp
    mainwindows.h
    mainwindows.cpp)

add_executable(${PROG_NAME} ${PROG_SRC}
    main.cpp)

target_link_libraries(${PROG_NAME} Qt5::Core)
target_link_libraries(${PROG_NAME} Qt5::Widgets)
target_link_libraries(${PROG_NAME} Qt5::Gui)

install (TARGETS ${PROG_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
