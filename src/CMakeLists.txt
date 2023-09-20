cmake_minimum_required(VERSION 3.5)

project(deepin-ai-daemon)

set(ANALYZESERVER_XML ${DBUS_XML_DIR}/org.deepin.ai.daemon.AnalyzeServer.xml)

pkg_search_module(NlGenl REQUIRED libnl-genl-3.0 IMPORTED_TARGET)
pkg_check_modules(Lucene REQUIRED IMPORTED_TARGET liblucene++ liblucene++-contrib)
pkg_check_modules(Taglib REQUIRED IMPORTED_TARGET taglib)
pkg_check_modules(ICU REQUIRED IMPORTED_TARGET icu-i18n)

find_package(DtkWidget REQUIRED)
find_package(DtkGui REQUIRED)
find_package(DtkCMake REQUIRED)
find_package(Qt5 COMPONENTS Widgets DBus REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(dtkocr REQUIRED)

FILE(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

FILE(GLOB_RECURSE ANALYZER_SRC
    ${CMAKE_SOURCE_DIR}/3rdparty/analyzer/*.cpp
    ${CMAKE_SOURCE_DIR}/3rdparty/analyzer/*.h
)

qt5_add_dbus_adaptor(SRC_FILES ${ANALYZESERVER_XML}
    server/analyzeserverdbus.h AnalyzeServerDBus)

add_executable(${PROJECT_NAME} ${ANALYZER_SRC} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${DtkWidget_INCLUDE_DIRS}
        ${DtkGUI_INCLUDE_DIRS}
        ${dtkocr_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/3rdparty
)

target_link_libraries(${PROJECT_NAME}
    Qt5::DBus
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    ${Boost_LIBRARIES}
    ${DtkWidget_LIBRARIES}
    ${DtkGUI_LIBRARIES}
    ${dtkocr_LIBRARIES}
    PkgConfig::NlGenl
    PkgConfig::Lucene
    PkgConfig::Taglib
    PkgConfig::ICU
    pthread
)

# bin
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# services
install(FILES
    ${CMAKE_SOURCE_DIR}/assets/autostart/org.deepin.ai.daemon.service
    ${CMAKE_SOURCE_DIR}/assets/autostart/org.deepin.ai.daemon.AnalyzeServer.service
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)

# systemd service
install(FILES ${CMAKE_SOURCE_DIR}/assets/autostart/deepin-ai-daemon.service
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/user)

# xdg-autostart
install(FILES ${CMAKE_SOURCE_DIR}/assets/autostart/deepin-ai-daemon.desktop
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart)

# install Manual App icon
install(FILES ${CMAKE_SOURCE_DIR}/assets/icon/deepin-ai-daemon.svg
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
