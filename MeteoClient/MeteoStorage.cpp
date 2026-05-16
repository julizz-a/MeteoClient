#include "MeteoStorage.h"
#include "MeteoTypes.h"
#include <algorithm>
#include <map>
#include <limits>

namespace meteo {

bool MeteoStorage::isWithinTimeRange(const WeatherRecord& record, int minutesBack) const {
    if (minutesBack <= 0) return true;
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::minutes(minutesBack);
    return record.timestamp >= cutoff;
}

void MeteoStorage::addRecord(const std::string& station, float temp, float hum, float wind) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.emplace_back(station, temp, hum, wind);
}

void MeteoStorage::addRecord(const WeatherData& data) {
    addRecord(data.stationName, data.temperature, data.humidity, data.windSpeed);
}

std::vector<WeatherRecord> MeteoStorage::getRecords(const std::string& station, int minutesBack) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<WeatherRecord> result;
    for (const auto& record : m_records) {
        if (record.stationName == station && isWithinTimeRange(record, minutesBack)) {
            result.push_back(record);
        }
    }
    return result;
}

std::vector<WeatherRecord> MeteoStorage::getAllRecords() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records;
}

std::vector<std::string> MeteoStorage::getStationNames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    for (const auto& record : m_records) {
        if (std::find(names.begin(), names.end(), record.stationName) == names.end()) {
            names.push_back(record.stationName);
        }
    }
    return names;
}

size_t MeteoStorage::getRecordsCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records.size();
}

void MeteoStorage::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.clear();
}

void MeteoStorage::cleanOldRecords(int minutesBack) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (minutesBack <= 0) return;
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::minutes(minutesBack);
    m_records.erase(
        std::remove_if(m_records.begin(), m_records.end(),
                       [cutoff](const WeatherRecord& record) {
                           return record.timestamp < cutoff;
                       }),
        m_records.end()
        );
}

std::vector<WeatherRecord> MeteoStorage::getLastRecords() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, WeatherRecord> lastForStation;
    for (const auto& record : m_records) {
        lastForStation[record.stationName] = record;
    }
    std::vector<WeatherRecord> result;
    for (const auto& pair : lastForStation) {
        result.push_back(pair.second);
    }
    return result;
}

MeteoStorage::StationStats MeteoStorage::getStationStats(const std::string& station) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    StationStats stats;
    stats.stationName = station;
    stats.avgTemperature = 0;
    stats.avgHumidity = 0;
    stats.avgWindSpeed = 0;
    stats.maxTemperature = -std::numeric_limits<float>::max();
    stats.minTemperature = std::numeric_limits<float>::max();
    stats.maxHumidity = -std::numeric_limits<float>::max();
    stats.minHumidity = std::numeric_limits<float>::max();
    stats.maxWindSpeed = -std::numeric_limits<float>::max();
    stats.minWindSpeed = std::numeric_limits<float>::max();
    stats.recordCount = 0;

    float sumTemp = 0, sumHum = 0, sumWind = 0;
    size_t count = 0;

    for (const auto& record : m_records) {
        if (record.stationName == station) {
            sumTemp += record.temperature;
            sumHum += record.humidity;
            sumWind += record.windSpeed;

            if (record.temperature > stats.maxTemperature) stats.maxTemperature = record.temperature;
            if (record.temperature < stats.minTemperature) stats.minTemperature = record.temperature;
            if (record.humidity > stats.maxHumidity) stats.maxHumidity = record.humidity;
            if (record.humidity < stats.minHumidity) stats.minHumidity = record.humidity;
            if (record.windSpeed > stats.maxWindSpeed) stats.maxWindSpeed = record.windSpeed;
            if (record.windSpeed < stats.minWindSpeed) stats.minWindSpeed = record.windSpeed;

            count++;
        }
    }

    if (count > 0) {
        stats.avgTemperature = sumTemp / count;
        stats.avgHumidity = sumHum / count;
        stats.avgWindSpeed = sumWind / count;
        stats.recordCount = count;
    }
    return stats;
}

} // namespace meteo
