#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <random>
#include <vector>
#include <string>
#include "MeteoStorage.h"

using namespace meteo;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    qDebug() << "\n=== ТЕСТОВЫЙ РЕЖИМ ===\n";

    MeteoStorage storage;

    // Генератор случайных чисел
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> tempDist(-10.0, 30.0);
    std::uniform_real_distribution<> humDist(40.0, 85.0);
    std::uniform_real_distribution<> windDist(0.0, 12.0);

    // Две станции
    std::vector<std::string> stations = {"Балтийск", "Гусев"};

    int generationCount = 0;
    const int MAX_GENERATIONS = 4;

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        generationCount++;
        qDebug() << "--- Генерация" << generationCount << "---";

        for (const auto& station : stations) {
            float temp = static_cast<float>(tempDist(gen));
            float hum = static_cast<float>(humDist(gen));
            float wind = static_cast<float>(windDist(gen));

            storage.addRecord(station, temp, hum, wind);

            qDebug() << QString::fromStdString(station)
                     << "| Темп:" << temp << "°C"
                     << "| Влаж:" << hum << "%"
                     << "| Ветер:" << wind << "м/с";
        }

        // После 4 генераций выводим статистику и завершаем
        if (generationCount >= MAX_GENERATIONS) {
            timer.stop();

            qDebug() << "\n=== СТАТИСТИКА ===\n";

            for (const auto& station : stations) {
                auto stats = storage.getStationStats(station);

                qDebug() << QString::fromStdString(station) << ":";
                qDebug() << "  Минимальная температура:" << stats.minTemperature << "°C";
                qDebug() << "  Максимальная температура:" << stats.maxTemperature << "°C";
                qDebug() << "  Минимальная влажность:" << stats.minHumidity << "%";
                qDebug() << "  Максимальная влажность:" << stats.maxHumidity << "%";
                qDebug() << "  Минимальный ветер:" << stats.minWindSpeed << "м/с";
                qDebug() << "  Максимальный ветер:" << stats.maxWindSpeed << "м/с";
                qDebug() << "";
            }

            qDebug() << "=== КОНЕЦ ===";
            QTimer::singleShot(100, &app, &QCoreApplication::quit);
        }
    });

    timer.start(2000);

    return app.exec();
}
