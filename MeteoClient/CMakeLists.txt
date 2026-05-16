cmake_minimum_required(VERSION 3.16)
project(MeteoClient VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ВАЖНО: используем прямые слеши / вместо обратных \
set(ASIO_PATH "C:/Users/ylian/OneDrive/Documents/MeteoClient/asio-include")

# Добавляем путь к include
include_directories(${ASIO_PATH})

add_executable(MeteoClient
    MeteoClient.cpp
    MeteoClient.h
    MeteoStorage.cpp
    MeteoStorage.h
    MeteoTypes.h
    WeatherRecord.cpp
)

# Определяем ASIO_STANDALONE
target_compile_definitions(MeteoClient PRIVATE ASIO_STANDALONE)

# Для Windows подключаем библиотеки сокетов
if(WIN32)
    target_link_libraries(MeteoClient PRIVATE ws2_32 wsock32)
endif()
