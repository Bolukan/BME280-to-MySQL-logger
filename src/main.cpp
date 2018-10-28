#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>                      // need for buggy BME280

#if defined(ESP8266)
	#include <ESP8266WiFi.h>
#elif defined(ESP32)
	#include <WiFi.h>
#endif

#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <BME280I2C.h>

const long SERIAL_BAUD = 115200;
const int SENSOR_INTERVAL_MILLIS = 60000;
const char INSERT_SQL[] = "INSERT INTO `sensor`.`BME280` (`sensorid`, `timestamp`, `humidity`, `pressure`, `temperature`) VALUES ('%d', CURRENT_TIMESTAMP, '%d', '%d', '%d');";

#include "secrets.h"
#ifndef SECRETS_H
	#define SECRETS_H
	const char WIFI_SSID[]              = "WIFI_SSID_HERE";
	const char WIFI_PASSWORD[]          = "WIFI_PASSWORD_HERE";

	const char MYSQL_USER[]             = "MYSQL_USER_HERE";
	const char MYSQL_PASSWORD[]         = "MYSQL_PASSWORD_HERE";
	IPAddress MYSQL_SERVERIP(192, 168, 1, 100);       // MySQL server IP
	const int MYSQL_SERVERPORT          = 3306;       // default 3306
#endif

/*
 * GENERAL
 */
unsigned long previousMillis;

/*
 * WIFI
 */
WiFiClient client;

/*
 * MYSQL
 */
MySQL_Connection conn(&client);
MySQL_Cursor* cursor;
bool dbIsConnected = false;
char query[200];

/*
 * BME280
 */

#if defined(ESP8266)
  uint32_t sensorID = ESP.getChipId();
#elif defined(ESP32)
 	uint32_t sensorID = ESP.getEfuseMac() & 0xffffff;
#endif

BME280I2C::Settings settings(
	BME280::OSR_X1,
	BME280::OSR_X1,
	BME280::OSR_X1,
	BME280::Mode_Forced,
	BME280::StandbyTime_1000ms,
	BME280::Filter_Off,
	BME280::SpiEnable_False,
	BME280I2C::I2CAddr_0x76 // I2C address. I2C specific.
);

BME280I2C bme(settings);

/*
 *  EVENT FUNCTIONS
 */

// Station connected
// WiFiEventStationModeConnected: String ssid, uint8 bssid[6], uint8 channel
void onSTAConnected(WiFiEventStationModeConnected stationInfo) {
	Serial.printf("Connected to %s\n", stationInfo.ssid.c_str ());
}

// Got IP
// WiFiEventStationModeGotIP: IPAddress ip, IPAddress mask, IPAddress gw
void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
	Serial.printf("Connected: %s\n", WiFi.status() == WL_CONNECTED ? "yes" : "no");
	Serial.printf("Got IP: %s\n", ipInfo.ip.toString().c_str());
}

// Station disconnected
// WiFiEventStationModeDisconnected: String ssid, uint8 bssid[6], WiFiDisconnectReason reason
// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiType.h
void onSTADisconnected(WiFiEventStationModeDisconnected disconnectInfo) {
	Serial.printf("Disconnected from SSID: %s\n", disconnectInfo.ssid.c_str());
	Serial.printf("Reason: %d\n", disconnectInfo.reason);
	// reboot
	ESP.restart();
}

/*
 *  FUNCTIONS
 */

void connectToWiFi()
{
	Serial.println(F("Connecting to WiFi ..."));
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMySQL()
{
	if (WiFi.status() != WL_CONNECTED) return;
	// if (!conn.connected()) { Serial.println("conn not connected"); return; }
	Serial.println(F("Starting MYSQL connection"));
	Serial.printf("SQL: Connecting to %s as %s ... : ", MYSQL_SERVERIP.toString().c_str(), (char *)MYSQL_USER);

	// boolean connect(IPAddress server, int port, char *user, char *password);
	if (conn.connect(MYSQL_SERVERIP, MYSQL_SERVERPORT, (char *)MYSQL_USER, (char *)MYSQL_PASSWORD)) {
		Serial.println(F("OK. CONNECTED"));
		// create MySQL cursor object
		cursor = new MySQL_Cursor(&conn);
		dbIsConnected = true;
	} else {
		Serial.println(F("FAILED."));
		// conn.close();
	}
}


void setup()
{
	// WiFi
	static WiFiEventHandler e1, e2, e3;

	// SERIAL
	Serial.begin(SERIAL_BAUD);
	Serial.println(F("Log sensordata (BME280) directly to MySQL database"));

	// WiFi
	Serial.println(F("Starting WiFi"));
	e1 = WiFi.onStationModeGotIP(onSTAGotIP);
	e2 = WiFi.onStationModeConnected(onSTAConnected);
	e3 = WiFi.onStationModeDisconnected(onSTADisconnected);
	connectToWiFi();

	// BME280
	Serial.println(F("Starting BME280"));
	Serial.printf("SensorID (based on ESP): %06X\n", sensorID);
	Wire.begin();
	if (!bme.begin()) {
		Serial.println(F("Could not find BME280 sensor"));
		Serial.println(F("You have 60 seconds to connect the wires ..."));
		delay(60000);
		ESP.restart();
	}
	if (bme.chipModel() != BME280::ChipModel_BME280) {
		Serial.println(F("Found BME280, but not full BME280 model"));
		Serial.println(F("You have 60 seconds to connect a full BME280 ..."));
		delay(60000);
		ESP.restart();
	}

	previousMillis = millis() - (SENSOR_INTERVAL_MILLIS - 10000); // 10 seconds before first measurement
}

void loop()
{
	if ((unsigned long)(millis() - previousMillis) >= SENSOR_INTERVAL_MILLIS) {
		previousMillis = millis();

		// check mysql connection
		if (!conn.connected()) {
			connectToMySQL();
			delay(1000);
		}

		if (conn.connected()) {
			if (dbIsConnected) {
				float temp(NAN), hum(NAN), pres(NAN);
				BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
				BME280::PresUnit presUnit(BME280::PresUnit_hPa);
				bme.read(pres, temp, hum, tempUnit, presUnit);

				uint32_t hum100  = (uint32_t)(hum * 100);
				uint32_t pres100 = (uint32_t)(pres * 100);
				int32_t temp100 = (int32_t)(temp * 100);
				sprintf(query, INSERT_SQL, sensorID, hum100, pres100, temp100);
				Serial.printf("Query %s\n", query);
				cursor->execute(query);
				//conn.close();
				//dbIsConnected = false;
			}
		} else {
			Serial.println(F("Logging missed."));
		}
	}
}
