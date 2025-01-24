set(LIB_NAME "lib${PROJECT_NAME}")

set(LIB_SRC
    documenttemplate.h
    documenttemplate.cpp
    documentitem.h
    documentitem.cpp
    documentdatainterface.h
    documentdatainterface.cpp
    documentdatainterface.h
    documentdatainterface.cpp
)

add_library(${LIB_NAME} ${LIB_SRC}
)

target_compile_definitions(${LIB_NAME}
  PRIVATE AUTOQUILLPP_LIBRARY)

target_link_libraries(${LIB_NAME} Qt5::Core)
target_link_libraries(${LIB_NAME} Qt5::Widgets)
target_link_libraries(${LIB_NAME} Qt5::Gui)

add_library(${STEVIAPP_SDK_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})

    target_include_directories(${LIB_NAME} PUBLIC
      $<INSTALL_INTERFACE:include>
    )

    install(TARGETS ${LIB_NAME}
      EXPORT ${STEVIAPP_SDK_EXPORTNAME}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      )

foreach(FILE IN LISTS LIB_SRC)
        if(FILE MATCHES "^((.*)/)?([^/]+)\\.h$")
                install(FILES ${FILE} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_MATCH_2})
        endif()
endforeach()

