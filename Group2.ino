#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <DHT.h>

const char* SSID = "Dialog 4G 640";
const char* PSK = "1d90cB6c";
const unsigned char INTERVAL_SECS = 30;
const unsigned char STATION_ID = 7;
const unsigned char DHT_PIN = 4;
const unsigned char CLR_PIN = 13;

struct Record {
    unsigned int uuid;
    unsigned long sentTimestamp;
    unsigned char stationId = STATION_ID;
    double temp;
    double humidity;
};

HTTPClient http;
DHT dht(DHT_PIN, DHT11);

String readFile(String path) {
    File file = SPIFFS.open(path);
    String content = file.readString();
    content.trim();
    file.close();
    return content;
}

void writeFile(String path, String content) {
    File file = SPIFFS.open(path, FILE_WRITE);
    file.println(content);
    file.close();
}

void writeRecord(Record record) {
    String content = "";
    content += "?1=";
    content += record.uuid;
    content += "&2=";
    content += record.sentTimestamp;
    content += "&3=";
    content += record.stationId;
    content += "&4=";
    content += record.temp;
    content += "&5=";
    content += record.humidity;

    String filePath = "/records/" + String(record.uuid) + ".txt";
    writeFile(filePath, content);
    Serial.println("WRITTEN: " + content + " TO: " + filePath);
}

int sendRequest(String url) {
    Serial.println("\nSENDING: " + url);

    http.begin(url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    Serial.println("HTTP CODE: " + String(httpCode));

    if (httpCode > 0) {
        Serial.println("RECEIVED: " +  http.getString());
    }
    http.end();

    return httpCode;
}

void sleepDeep(int secs) {
    Serial.println("\nINITIATING: Deep sleep FOR: " + String(secs) + "s" );
    esp_sleep_enable_timer_wakeup(secs * 1000000);
    Serial.println("==================================================================");
    esp_deep_sleep_start();
}

bool timeConfigured;

Record generateRecord() {
    Record record;

    record.temp = dht.readTemperature();
    record.humidity = dht.readHumidity();

    long lastTimestamp;
    if (timeConfigured) {
        lastTimestamp = time(NULL);
    } else {
        lastTimestamp = readFile("/timestamp_last.txt").toInt() + INTERVAL_SECS;
    }
    record.sentTimestamp = lastTimestamp;
    writeFile("/timestamp_last.txt", String(lastTimestamp));

    int nextUuid = readFile("/uuid_next.txt").toInt();
    record.uuid = nextUuid;
    writeFile("/uuid_next.txt", String(nextUuid+1));

    return record;
}

int wiFiConnectTimeLeft;

void setup() {
    Serial.begin(115200);
    Serial.println("\n==================================================================");
    Serial.println("CONFIGURED: Serial communication AT: 115200");

    SPIFFS.begin(true);
    if (digitalRead(CLR_PIN) == HIGH) {
        Serial.println("INITIATING: Memory wipe BECAUSE: Memory wipe pin is high");

        if (SPIFFS.remove("/uuid_next.txt")) {
            Serial.println("REMOVED: /uuid_next.txt");
        }
        if (SPIFFS.remove("/timestamp_last.txt")) {
            Serial.println("REMOVED: /uuid_next.txt");
        }

        File recordsDir = SPIFFS.open("/records");
        File nextRecordFile;
        if (recordsDir) {
            while (nextRecordFile = recordsDir.openNextFile()) {
                String nextRecordFilePath = "/records/" + String(nextRecordFile.name());
                if (SPIFFS.remove(nextRecordFilePath)) {
                    Serial.println("REMOVED: " + nextRecordFilePath);
                }
            }
        }
        Serial.println("CONFIGURED: Storage DO: Reset chip");
        while(true);
    } else {
        if (!SPIFFS.exists("/uuid_next.txt")) {
            writeFile("/uuid_next.txt", "0");
            Serial.println("WRITTEN: /uuid_next.txt CONTENT: 0 BECAUSE: File not found");
        }
        if (!SPIFFS.exists("/timestamp_last.txt")) {
            writeFile("/timestamp_last.txt", "0");
            Serial.println("WRITTEN: /timestamp_last.txt CONTENT: 0 BECAUSE: File not found");
        }
        if (!SPIFFS.open("/records")) {
            Serial.println("CREATED: /records BECAUSE: Directory not found");
        }
        Serial.println("CONFIGURED: Storage");
    }

    Serial.print("CONFIGURING: WiFi WITH: " + String(SSID) + " ");
    wiFiConnectTimeLeft = 20;
    WiFi.begin(SSID, PSK);
    while (WiFi.status() != WL_CONNECTED && wiFiConnectTimeLeft > 0) {
        Serial.print(".");
        wiFiConnectTimeLeft--;
        delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nCONFIGURED: WiFi WITH: " + String(SSID));
    } else {
        Serial.println("\nERR: Configuring WiFi WITH: " + String(SSID) + " BECAUSE: Timed out");
    }

    dht.begin();
    Serial.println("CONFIGURED: DHT11");
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        configTime(19800, 0, "pool.ntp.org");
        timeConfigured = true;
        Serial.println("CONFIGURED: System time WITH: pool.ntp.org");
    } else {
        timeConfigured = false;
        Serial.println("ERR: Configuring system time WITH: pool.ntp.org BECAUSE: No WiFi");
    }

    Record record = generateRecord();
    writeRecord(record);

    File recordsDir = SPIFFS.open("/records");
    File recordFile;
    while (WiFi.status() == WL_CONNECTED && (recordFile = recordsDir.openNextFile())) {
        if(!recordFile.isDirectory()){
            String recordFilePath = "/records/" + String(recordFile.name());
            String url = "https://script.google.com/macros/s/AKfycbzFn3jyi1D9cwyo8ITDkE05curOmnXlc-S8IGseX834o7Sl7tFYaDBmHdoleGkFai-qIg/exec" + readFile(recordFilePath);
            int httpCode = sendRequest(url);
            if (httpCode == 200) {
                SPIFFS.remove(recordFilePath);
            }
        }
    }

    sleepDeep(INTERVAL_SECS);
}
