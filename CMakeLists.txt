# Copyright (C) 2022 The Qt Company Ltd.
# Copyright 2024 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.14)

project(imx-video-to-texture VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick)

pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_search_module(gstreamer-gl REQUIRED IMPORTED_TARGET gstreamer-gl-1.0)

set(PROJECT_SOURCES
        cpp/gltexturerenderer.cpp cpp/gltexturerenderer.hpp
        cpp/gstplayer.cpp cpp/gstplayer.hpp
        cpp/main.cpp
        cpp/mediastream.cpp cpp/mediastream.hpp
        cpp/mediascreenshot.cpp cpp/mediascreenshot.hpp
        qrc/image.qrc
        qrc/icons.qrc
        qrc/qml.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(imx-video-to-texture
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
endif()

target_compile_definitions(imx-video-to-texture
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(imx-video-to-texture
  PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Quick
)

target_include_directories(imx-video-to-texture PUBLIC cpp/)

target_link_libraries(imx-video-to-texture
    PRIVATE
    PkgConfig::gstreamer
    PkgConfig::gstreamer-gl
)

qt_add_qml_module(imx-video-to-texture
    URI ImxVideoToTexture
    VERSION 1.0
    RESOURCE_PREFIX /qml
)

set_target_properties(imx-video-to-texture PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(imx-video-to-texture)
    qt_finalize_executable(imx-video-to-texture)
endif()
