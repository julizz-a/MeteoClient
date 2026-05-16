cmake_minimum_required(VERSION 3.16)

project(meteo_project LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# ===== ДОБАВЛЯЕМ ПУТИ К METEO CLIENT =====
set(METEO_CLIENT_DIR "C:/Users/ylian/OneDrive/Documents/MeteoClient")
include_directories(${METEO_CLIENT_DIR})

# Путь к ASIO
set(ASIO_INCLUDE_DIR "${METEO_CLIENT_DIR}/asio-include")
include_directories(${ASIO_INCLUDE_DIR})

add_executable(meteo_project
    main.cpp
    # Добавляем исходники MeteoClient
    ${METEO_CLIENT_DIR}/MeteoClient.cpp
    ${METEO_CLIENT_DIR}/MeteoStorage.cpp
    ${METEO_CLIENT_DIR}/WeatherRecord.cpp
)

# Определяем ASIO_STANDALONE
target_compile_definitions(meteo_project PRIVATE ASIO_STANDALONE)

target_link_libraries(meteo_project PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)

# Для Windows подключаем библиотеки сокетов
if(WIN32)
    target_link_libraries(meteo_project PRIVATE ws2_32 wsock32)
endif()

include(GNUInstallDirs)
install(TARGETS meteo_project
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
