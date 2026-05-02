// ==========================================================================
//  QBIT -- Bluetooth phone time sync
// ==========================================================================
#include "bluetooth_time.h"
#include "settings.h"
#include "time_manager.h"

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

#if defined(ESP32) && __has_include(<BLEDevice.h>)
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#define QBIT_BLE_TIME_ENABLED 1
#else
#define QBIT_BLE_TIME_ENABLED 0
#endif

// Custom QBIT Time Service.
// Write ASCII to the time characteristic:
//   1777777777
//   1777777777000
//   1777777777|America/Toronto
#define QBIT_TIME_SERVICE_UUID "7f5c1000-6f33-4f0d-a9d7-51d3d7d8b001"
#define QBIT_TIME_WRITE_UUID   "7f5c1001-6f33-4f0d-a9d7-51d3d7d8b001"
#define QBIT_TIME_STATUS_UUID  "7f5c1002-6f33-4f0d-a9d7-51d3d7d8b001"

#if QBIT_BLE_TIME_ENABLED
static BLECharacteristic *s_statusChar = nullptr;

static void updateStatusCharacteristic(const char *status) {
    if (!s_statusChar) return;
    String value = String(status) + "|" + timeManagerGetISO8601();
    s_statusChar->setValue((uint8_t *)value.c_str(), value.length());
}

static bool applyEpochSeconds(uint64_t epochSeconds) {
    if (epochSeconds < 1000000000ULL) {
        return false;
    }
    struct timeval tv = {};
    tv.tv_sec = (time_t)epochSeconds;
    tv.tv_usec = 0;
    return settimeofday(&tv, nullptr) == 0;
}

class TimeWriteCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) override {
        auto rawValue = characteristic->getValue();
        String payload(rawValue.c_str());
        payload.trim();
        if (payload.length() == 0) {
            updateStatusCharacteristic("empty");
            return;
        }

        int sep = payload.indexOf('|');
        String epochText = sep >= 0 ? payload.substring(0, sep) : payload;
        String tzText = sep >= 0 ? payload.substring(sep + 1) : "";
        epochText.trim();
        tzText.trim();

        char *end = nullptr;
        uint64_t epoch = strtoull(epochText.c_str(), &end, 10);
        if (end == epochText.c_str()) {
            updateStatusCharacteristic("bad-time");
            return;
        }
        if (epoch > 100000000000ULL) {
            epoch /= 1000ULL;
        }

        if (!applyEpochSeconds(epoch)) {
            updateStatusCharacteristic("bad-time");
            return;
        }

        if (tzText.length() > 0) {
            timeManagerSetTimezone(tzText);
            saveSettings();
        }

        Serial.printf("[BLE Time] Synced epoch=%llu tz=%s\n",
                      (unsigned long long)epoch, tzText.c_str());
        updateStatusCharacteristic("synced");
    }
};

class TimeServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *server) override {
        (void)server;
        updateStatusCharacteristic("connected");
    }

    void onDisconnect(BLEServer *server) override {
        (void)server;
        BLEDevice::startAdvertising();
    }
};
#endif

void bluetoothTimeInit() {
#if QBIT_BLE_TIME_ENABLED
    String name = "QBIT-" + getDeviceId().substring(0, 4);
    BLEDevice::init(name.c_str());

    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new TimeServerCallbacks());

    BLEService *service = server->createService(QBIT_TIME_SERVICE_UUID);

    BLECharacteristic *timeWrite = service->createCharacteristic(
        QBIT_TIME_WRITE_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    timeWrite->setCallbacks(new TimeWriteCallbacks());

    s_statusChar = service->createCharacteristic(
        QBIT_TIME_STATUS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    s_statusChar->addDescriptor(new BLE2902());
    updateStatusCharacteristic("ready");

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(QBIT_TIME_SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.printf("[BLE Time] Advertising as %s\n", name.c_str());
#else
    Serial.println("[BLE Time] BLE library unavailable; Bluetooth time sync disabled");
#endif
}
