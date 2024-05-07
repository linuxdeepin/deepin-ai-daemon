cmake_minimum_required(VERSION 3.5)

project(deepin-ai-daemon)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 内存泄漏检测
# set(CMAKE_CXX_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
# set(CMAKE_C_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
# set(CMAKE_Ｌ_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")

# 执行 git submodule update --init 命令
#execute_process(
#    COMMAND git submodule update --init --recursive
#    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#    RESULT_VARIABLE SUBMODULE_UPDATE_RESULT
#)

## 检查子模块更新是否成功
#if(NOT SUBMODULE_UPDATE_RESULT EQUAL "0")
#    message(FATAL_ERROR "git submodule update --init failed: ${SUBMODULE_UPDATE_RESULT}")
#endif()

find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif()

# dbus xml dir
set(DBUS_XML_DIR ${CMAKE_SOURCE_DIR}/assets/dbus)

add_subdirectory(3rdparty)
add_subdirectory(src)



