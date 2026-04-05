cmake_minimum_required(VERSION 3.14)
project(backend_notes)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZMQ QUIET libzmq)

find_library(ZMQ_LIBRARY
        NAMES zmq libzmq
        PATHS ${PC_ZMQ_LIBRARY_DIRS} /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
        REQUIRED
)

find_path(ZMQ_INCLUDE_DIR
        NAMES zmq.h zmq.hpp
        PATHS ${PC_ZMQ_INCLUDE_DIRS} /usr/include /usr/local/include
        REQUIRED
)

find_file(ZMQ_HPP
        NAMES zmq.hpp
        PATHS ${ZMQ_INCLUDE_DIR} ${ZMQ_INCLUDE_DIR}/zmq
        REQUIRED
)

find_path(NLOHMANN_JSON_INCLUDE_DIR
        NAMES nlohmann/json.hpp
        PATHS /usr/include /usr/local/include /usr/include/nlohmann
        REQUIRED
)

if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    pkg_check_modules(PC_NLOHMANN_JSON QUIET nlohmann_json)
    find_path(NLOHMANN_JSON_INCLUDE_DIR
            NAMES nlohmann/json.hpp
            PATHS ${PC_NLOHMANN_JSON_INCLUDE_DIRS}
            REQUIRED
    )
endif()

include_directories(${INCLUDE_DIR})
include_directories(${ZMQ_INCLUDE_DIR})
include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(imgui STATIC
        ${THIRD_PARTY_DIR}/imgui/imgui.cpp
        ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
        ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
        ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
        ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
        ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
        ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
        ${THIRD_PARTY_DIR}/imgui
        ${THIRD_PARTY_DIR}/imgui/backends
)
target_compile_options(imgui PRIVATE -fPIC)

add_library(app_core STATIC
        src/gui.cpp
        src/server.cpp
        src/json.cpp
)
target_include_directories(app_core PUBLIC
        ${INCLUDE_DIR}
        ${THIRD_PARTY_DIR}/imgui
        ${THIRD_PARTY_DIR}/imgui/backends
        ${THIRD_PARTY_DIR}/implot
        ${PostgreSQL_INCLUDE_DIRS}
        /usr/include/postgresql
)
target_link_libraries(app_core PUBLIC imgui)

add_library(implot STATIC
        ${THIRD_PARTY_DIR}/implot/implot.cpp
        ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
        ${THIRD_PARTY_DIR}/implot/implot_items.cpp
)
target_include_directories(implot PUBLIC ${THIRD_PARTY_DIR}/implot)
target_link_libraries(implot PRIVATE imgui)
target_compile_options(implot PRIVATE -fPIC)


add_executable(main
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/database.cpp
)


find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)

target_link_libraries(main PRIVATE
        app_core
        imgui
        implot
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${ZMQ_LIBRARY}
        Threads::Threads
        PostgreSQL::PostgreSQL
)


if (UNIX AND NOT APPLE)
    target_link_libraries(main PRIVATE rt dl)
endif()


target_include_directories(main PRIVATE
        ${INCLUDE_DIR}
        ${ZMQ_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
)


target_compile_definitions(main PRIVATE
        _GLIBCXX_USE_CXX11_ABI=1
)
