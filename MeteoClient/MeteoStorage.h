#ifndef METEO_STORAGE_H
#define METEO_STORAGE_H

#include "MeteoTypes.h"
#include <vector>
#include <string>
#include <mutex>

namespace meteo {

class MeteoStorage {
public:
    MeteoStorage() = default;
    ~MeteoStorage() = default;

    // Запрещаем копирование
    MeteoStorage(const MeteoStorage&) = delete;
    MeteoStorage& operator=(const MeteoStorage&) = delete;

    // Основные методы
    void addRecord(const std::string& station, float temp, float hum, float wind);
    void addRecord(const WeatherData& data);
    std::vector<WeatherRecord> getRecords(const std::string& station, int minutesBack = 0) const;
    std::vector<WeatherRecord> getAllRecords() const;
    std::vector<std::string> getStationNames() const;

    // Вспомогательные методы
    size_t getRecordsCount() const;
    void clear();
    void cleanOldRecords(int minutesBack);
    std::vector<WeatherRecord> getLastRecords() const;

    // Статистика
    struct StationStats {
        std::string stationName;
        float avgTemperature;
        float avgHumidity;
        float avgWindSpeed;
        float maxTemperature;
        float minTemperature;
        float maxHumidity;
        float minHumidity;
        float maxWindSpeed;
        float minWindSpeed;
        size_t recordCount;
    };
    StationStats getStationStats(const std::string& station) const;

private:
    mutable std::mutex m_mutex;
    std::vector<WeatherRecord> m_records;
    bool isWithinTimeRange(const WeatherRecord& record, int minutesBack) const;
};

} // namespace meteo

#endif
