#ifndef METEO_TYPES_H
#define METEO_TYPES_H

#include <string>
#include <functional>
#include <chrono>

namespace meteo {

// Структура записи погоды (только объявления)
struct WeatherRecord {
    std::string stationName;
    float temperature;
    float humidity;
    float windSpeed;
    std::chrono::system_clock::time_point timestamp;

    // Только объявления конструкторов и метода
    WeatherRecord();
    WeatherRecord(const std::string& name, float temp, float hum, float wind);
    std::string getTimeOnly() const;
};

// Статус подключения
enum class ConnectionStatus {
    Disconnected,
    Connecting,
    Connected,
    Error,
    Timeout
};

// Типы данных для погоды
struct WeatherData {
    std::string stationName;
    float temperature;
    float humidity;
    float windSpeed;

    WeatherData() : temperature(0), humidity(0), windSpeed(0) {}
    WeatherData(const std::string& name, float temp, float hum, float wind)
        : stationName(name), temperature(temp), humidity(hum), windSpeed(wind) {}
};

// Колбэки
using DataReceivedCallback = std::function<void(const WeatherData&)>;
using StatusChangedCallback = std::function<void(const std::string&, ConnectionStatus)>;

} // namespace meteo

#endif
