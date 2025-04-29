cmake_minimum_required(VERSION 3.16)

project(LaminateSketch VERSION 0.1 LANGUAGES CXX)

# Устанавливаем политики для новых версий CMake
cmake_policy(SET CMP0167 NEW)

# Включаем модуль для работы с внешними проектами
include(FetchContent)

# Настройки для загрузки libdxfrw
FetchContent_Declare(
    libdxfrw
    GIT_REPOSITORY https://github.com/codelibs/libdxfrw.git
    GIT_TAG        master
)

# Форсируем настройки для libdxfrw
set(USE_ICONV ON CACHE BOOL "Use iconv" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static build" FORCE)

# Загружаем и собираем libdxfrw
FetchContent_MakeAvailable(libdxfrw)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Поиск зависимостей
find_package(Boost REQUIRED COMPONENTS headers)

# Указываем путь к MSYS2/MINGW, если используется
set(ICONV_ROOT "C:/Dev/msys64/mingw64")

# Поиск iconv
find_path(ICONV_INCLUDE_DIR iconv.h
    PATHS ${ICONV_ROOT}/include /mingw64/include /usr/include
)
find_library(ICONV_LIBRARY
    NAMES iconv libiconv iconv.dll libiconv.dll.a libiconv.lib
    PATHS ${ICONV_ROOT}/lib /mingw64/lib /usr/lib
)

if(NOT ICONV_INCLUDE_DIR OR NOT ICONV_LIBRARY)
    message(FATAL_ERROR "libiconv not found. ICONV_INCLUDE_DIR=${ICONV_INCLUDE_DIR}, ICONV_LIBRARY=${ICONV_LIBRARY}")
endif()

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(LaminateSketch
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        common.h sketch.h
        common.cpp sketch.cpp
        dx_data.h dx_iface.h handler.h
        dx_iface.cpp handler.cpp
        resources.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET LaminateSketch APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(LaminateSketch SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(LaminateSketch
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(LaminateSketch PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Директории с заголовками
target_include_directories(${PROJECT_NAME} PRIVATE
    ${libdxfrw_SOURCE_DIR}/include
    ${libdxfrw_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${ICONV_INCLUDE_DIR}
)

# Линковка
target_link_libraries(${PROJECT_NAME} PRIVATE
    dxfrw
    ${Boost_LIBRARIES}
    ${ICONV_LIBRARY}
)

# Для Windows
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        stdc++fs
    )
    add_definitions(-DLIBICONV_PLUG -DICONV_CONST=)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LaminateSketch)
endif()
set_target_properties(LaminateSketch PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS LaminateSketch
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LaminateSketch)
endif()

# Платформозависимые настройки иконки
if (WIN32)
    # Для Windows: добавляем .rc-файл
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.ico
        ${CMAKE_CURRENT_BINARY_DIR}/app_icon.ico
        COPYONLY
    )
    set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/app_icon.ico)
    target_sources(LaminateSketch PRIVATE ${RC_FILE})

    # Генерируем .rc-файл
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/icon.rc
        "IDI_ICON1 ICON DISCARDABLE \"app_icon.ico\"\n"
    )
    target_sources(LaminateSketch PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/icon.rc)

elseif (APPLE)
    # Для macOS: указываем ICNS-файл
    set(MACOSX_BUNDLE_ICON_FILE app_icon.icns)
    set_source_files_properties(
        icons/app_icon.icns
        PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(LaminateSketchp PRIVATE icons/app_icon.icns)
    set_target_properties(LaminateSketch PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON ${MACOSX_BUNDLE_ICON_FILE}
    )
endif()
