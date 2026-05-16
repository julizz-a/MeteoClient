#ifndef METEO_CLIENT_H
#define METEO_CLIENT_H

#include "MeteoTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

namespace meteo {

class MeteoClientImpl;

class MeteoClient {
public:
    MeteoClient();
    ~MeteoClient();

    // Управление станциями
    void addStation(const std::string& name, unsigned short port);
    void removeStation(const std::string& name);
    void clearStations();
    std::vector<std::string> getStationNames() const;

    // Управление опросом
    void startPolling(int intervalSeconds = 10);
    void stopPolling();
    void pollStation(const std::string& name);
    void pollAllStations();

    // Колбэки
    void setDataCallback(DataReceivedCallback callback);
    void setStatusCallback(StatusChangedCallback callback);

    // Подключение к хранилищу (опционально)
    void setStorage(class MeteoStorage* storage);

private:
    std::unique_ptr<MeteoClientImpl> pImpl;
};

} // namespace meteo

#endif
