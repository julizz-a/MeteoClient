#include "MeteoClient.h"
#include "MeteoStorage.h"
#include <asio.hpp>
#include <thread>
#include <chrono>
#include <map>
#include <sstream>
#include <atomic>

namespace meteo {

// Внутренняя структура станции
struct InternalStation {
    std::string name;
    unsigned short port;
    std::shared_ptr<asio::ip::tcp::socket> socket;
    bool isPolling;
    ConnectionStatus status;
    mutable std::mutex mutex;

    InternalStation(const std::string& n, unsigned short p)
        : name(n), port(p), isPolling(false), status(ConnectionStatus::Disconnected) {}
};

// Реализация клиента
class MeteoClientImpl {
public:
    MeteoClientImpl() : m_running(false), m_intervalSeconds(10), m_storage(nullptr) {
        m_ioContext = std::make_shared<asio::io_context>();
        m_workGuard = std::make_shared<asio::executor_work_guard<asio::io_context::executor_type>>(
            asio::make_work_guard(*m_ioContext)
            );
    }

    ~MeteoClientImpl() {
        stopPolling();
    }

    void setStorage(MeteoStorage* storage) {
        m_storage = storage;
    }

    void addStation(const std::string& name, unsigned short port) {
        std::lock_guard<std::mutex> lock(m_stationsMutex);
        if (m_stations.find(name) == m_stations.end()) {
            m_stations[name] = std::make_shared<InternalStation>(name, port);
            if (m_statusCallback) {
                m_statusCallback(name, ConnectionStatus::Disconnected);
            }
        }
    }

    void removeStation(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_stationsMutex);
        m_stations.erase(name);
    }

    void clearStations() {
        std::lock_guard<std::mutex> lock(m_stationsMutex);
        m_stations.clear();
    }

    std::vector<std::string> getStationNames() const {
        std::lock_guard<std::mutex> lock(m_stationsMutex);
        std::vector<std::string> names;
        for (const auto& pair : m_stations) {
            names.push_back(pair.first);
        }
        return names;
    }

    void startPolling(int intervalSeconds) {
        m_intervalSeconds = intervalSeconds;
        m_running = true;

        m_ioThread = std::thread([this]() {
            m_ioContext->run();
        });

        m_pollingThread = std::thread([this]() {
            while (m_running) {
                pollAllStations();
                std::this_thread::sleep_for(std::chrono::seconds(m_intervalSeconds));
            }
        });
    }

    void stopPolling() {
        m_running = false;
        if (m_workGuard) {
            m_workGuard->reset();
        }
        if (m_ioContext) {
            m_ioContext->stop();
        }
        if (m_ioThread.joinable()) {
            m_ioThread.join();
        }
        if (m_pollingThread.joinable()) {
            m_pollingThread.join();
        }
    }

    void pollStation(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_stationsMutex);
        auto it = m_stations.find(name);
        if (it != m_stations.end()) {
            pollStationInternal(it->second);
        }
    }

    void pollAllStations() {
        std::lock_guard<std::mutex> lock(m_stationsMutex);
        for (auto& pair : m_stations) {
            if (!pair.second->isPolling) {
                pollStationInternal(pair.second);
            }
        }
    }

    void setDataCallback(DataReceivedCallback callback) {
        m_dataCallback = callback;
    }

    void setStatusCallback(StatusChangedCallback callback) {
        m_statusCallback = callback;
    }

private:
    void pollStationInternal(std::shared_ptr<InternalStation> station) {
        if (station->isPolling) return;

        station->isPolling = true;
        station->status = ConnectionStatus::Connecting;

        if (m_statusCallback) {
            m_statusCallback(station->name, ConnectionStatus::Connecting);
        }

        auto socket = std::make_shared<asio::ip::tcp::socket>(*m_ioContext);
        station->socket = socket;

        asio::ip::tcp::endpoint endpoint(
            asio::ip::make_address("127.0.0.1"),
            station->port
            );

        socket->async_connect(endpoint, [this, station, socket](std::error_code ec) {
            if (!ec) {
                station->status = ConnectionStatus::Connected;
                if (m_statusCallback) {
                    m_statusCallback(station->name, ConnectionStatus::Connected);
                }
                requestData(station);
            } else {
                // ИСПРАВЛЕНО: закрываем сокет при ошибке
                station->status = ConnectionStatus::Error;
                station->isPolling = false;
                if (socket && socket->is_open()) {
                    socket->close();
                }
                if (m_statusCallback) {
                    m_statusCallback(station->name, ConnectionStatus::Error);
                }
            }
        });
    }

    void requestData(std::shared_ptr<InternalStation> station) {
        if (!station->socket || !station->socket->is_open()) return;

        std::string request = "GET\n";
        asio::async_write(*station->socket, asio::buffer(request),
                          [this, station](std::error_code ec, size_t /*length*/) {
                              if (!ec) {
                                  readResponse(station);
                              } else {
                                  station->isPolling = false;
                                  station->status = ConnectionStatus::Error;
                                  if (station->socket && station->socket->is_open()) {
                                      station->socket->close();
                                  }
                                  if (m_statusCallback) {
                                      m_statusCallback(station->name, ConnectionStatus::Error);
                                  }
                              }
                          });
    }

    void readResponse(std::shared_ptr<InternalStation> station) {
        auto buffer = std::make_shared<std::array<char, 1024>>();

        station->socket->async_read_some(asio::buffer(*buffer),
                                         [this, station, buffer](std::error_code ec, size_t length) {
                                             if (!ec && length > 0) {
                                                 std::string response(buffer->data(), length);
                                                 parseResponse(station->name, response);
                                             }
                                             station->isPolling = false;
                                             station->status = ConnectionStatus::Disconnected;
                                             if (station->socket && station->socket->is_open()) {
                                                 station->socket->close();
                                             }
                                         });
    }

    void parseResponse(const std::string& stationName, const std::string& response) {
        std::istringstream iss(response);
        float temp, hum, wind;

        if (iss >> temp >> hum >> wind) {
            WeatherData data(stationName, temp, hum, wind);

            if (m_storage) {
                m_storage->addRecord(data);
            }

            if (m_dataCallback) {
                m_dataCallback(data);
            }
        }

        if (m_statusCallback) {
            m_statusCallback(stationName, ConnectionStatus::Disconnected);
        }
    }

    std::shared_ptr<asio::io_context> m_ioContext;
    std::shared_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_workGuard;
    std::thread m_ioThread;
    std::thread m_pollingThread;
    std::atomic<bool> m_running;
    int m_intervalSeconds;

    std::map<std::string, std::shared_ptr<InternalStation>> m_stations;
    mutable std::mutex m_stationsMutex;

    DataReceivedCallback m_dataCallback;
    StatusChangedCallback m_statusCallback;
    MeteoStorage* m_storage;
};

// Реализация публичного интерфейса
MeteoClient::MeteoClient() : pImpl(std::make_unique<MeteoClientImpl>()) {}
MeteoClient::~MeteoClient() = default;

void MeteoClient::addStation(const std::string& name, unsigned short port) {
    pImpl->addStation(name, port);
}

void MeteoClient::removeStation(const std::string& name) {
    pImpl->removeStation(name);
}

void MeteoClient::clearStations() {
    pImpl->clearStations();
}

std::vector<std::string> MeteoClient::getStationNames() const {
    return pImpl->getStationNames();
}

void MeteoClient::startPolling(int intervalSeconds) {
    pImpl->startPolling(intervalSeconds);
}

void MeteoClient::stopPolling() {
    pImpl->stopPolling();
}

void MeteoClient::pollStation(const std::string& name) {
    pImpl->pollStation(name);
}

void MeteoClient::pollAllStations() {
    pImpl->pollAllStations();
}

void MeteoClient::setDataCallback(DataReceivedCallback callback) {
    pImpl->setDataCallback(callback);
}

void MeteoClient::setStatusCallback(StatusChangedCallback callback) {
    pImpl->setStatusCallback(callback);
}

void MeteoClient::setStorage(MeteoStorage* storage) {
    pImpl->setStorage(storage);
}

} // namespace meteo
