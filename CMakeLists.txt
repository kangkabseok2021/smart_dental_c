cmake_minimum_required(VERSION 3.21)

project(SmartDentalHandpiece VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_VERSION "${PROJECT_VERSION}")
endif()
if(GIT_VERSION STREQUAL "")
    set(GIT_VERSION "${PROJECT_VERSION}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Sql Charts)

add_compile_definitions(APP_VERSION="${GIT_VERSION}")

set(APP_SOURCES
    src/main.cpp
    src/HardwareHandler.h
    src/HardwareHandler.cpp
    src/StateManager.h
    src/StateManager.cpp
    src/TelemetryBridge.h
    src/TelemetryBridge.cpp
    src/DatabaseManager.h
    src/DatabaseManager.cpp
)

qt_add_executable(SmartDentalHandpiece ${APP_SOURCES} qml/qml.qrc)

target_include_directories(SmartDentalHandpiece PRIVATE src)

target_link_libraries(SmartDentalHandpiece
    PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Sql Qt6::Charts
)

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
