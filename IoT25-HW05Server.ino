#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>

// BLE server name
#define bleServerName "BME280_ESP32"

// DHT 설정
#define DHTPIN 4          // DHT11 데이터 핀 연결한 GPIO 번호
#define DHTTYPE DHT11     // 센서 종류 지정
DHT dht(DHTPIN, DHTTYPE);

float temp;
float hum;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3000;

bool deviceConnected = false;

// UUIDs
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
BLECharacteristic dhtTemperatureCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor dhtTemperatureDescriptor(BLEUUID((uint16_t)0x2902));

BLECharacteristic dhtHumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor dhtHumidityDescriptor(BLEUUID((uint16_t)0x2903));

// BLE 연결 콜백
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

void setup() {
  Serial.begin(115200);
  dht.begin();  // DHT 센서 시작

  // BLE 초기화
  BLEDevice::init(bleServerName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *dhtService = pServer->createService(SERVICE_UUID);

  // 온도
  dhtService->addCharacteristic(&dhtTemperatureCharacteristics);
  dhtTemperatureDescriptor.setValue("DHT11 temperature Celsius");
  dhtTemperatureCharacteristics.addDescriptor(&dhtTemperatureDescriptor);

  // 습도
  dhtService->addCharacteristic(&dhtHumidityCharacteristics);
  dhtHumidityDescriptor.setValue("DHT11 humidity");
  dhtHumidityCharacteristics.addDescriptor(&dhtHumidityDescriptor);

  dhtHumidityCharacteristics.addDescriptor(new BLE2902());

  dhtService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a client connection...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {
      // DHT11에서 읽기
      temp = dht.readTemperature(); // 섭씨
      hum = dht.readHumidity();

      if (isnan(temp) || isnan(hum)) {
        Serial.println("DHT 센서에서 값을 읽지 못했습니다!");
        return;
      }

      // 온도 전송
      static char temperatureChar[6];
      dtostrf(temp, 6, 2, temperatureChar);
      dhtTemperatureCharacteristics.setValue(temperatureChar);
      dhtTemperatureCharacteristics.notify();
      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.print(" °C");

      static char humidityChar[8];  // 여유있는 크기
      dtostrf(hum, 6, 2, humidityChar);
      // 종료문자 직접 삽입은 제거, dtostrf가 이미 처리
      Serial.println(humidityChar);

      dhtHumidityCharacteristics.setValue(humidityChar); 
      dhtHumidityCharacteristics.notify();

      Serial.print(" - Humidity: ");
      Serial.print(hum);
      Serial.println(" %");

      lastTime = millis();
    }
  }
}
