#include "MeteoTypes.h"
#include <chrono>
#include <ctime>
#include <cstring>

#ifdef _WIN32
#define localtime_r(_Time, _Tm) localtime_s(_Tm, _Time)
#endif

namespace meteo {

// Реализация конструкторов и метода WeatherRecord
WeatherRecord::WeatherRecord()
    : temperature(0), humidity(0), windSpeed(0) {
    timestamp = std::chrono::system_clock::now();
}

WeatherRecord::WeatherRecord(const std::string& name, float temp, float hum, float wind)
    : stationName(name), temperature(temp), humidity(hum), windSpeed(wind) {
    timestamp = std::chrono::system_clock::now();
}

std::string WeatherRecord::getTimeOnly() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    char buffer[9];
    struct tm timeinfo;
    localtime_r(&time_t, &timeinfo);
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return std::string(buffer);
}

} // namespace meteo
