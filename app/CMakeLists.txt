set(PROG_SRC
    main.cpp
    mainwindows.h
	mainwindows.cpp
	documentpreviewwidget.h
	documentpreviewwidget.cpp)

add_executable(${PROG_NAME} ${PROG_SRC}
    main.cpp)

target_link_libraries(${PROG_NAME} ${LIB_NAME})

target_link_libraries(${PROG_NAME} Qt5::Core)
target_link_libraries(${PROG_NAME} Qt5::Widgets)
target_link_libraries(${PROG_NAME} Qt5::Gui)
target_link_libraries(${PROG_NAME} Qt5::Gui)
target_link_libraries(${PROG_NAME} Qt5::Svg)

install (TARGETS ${PROG_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
