// secrets.h
#ifndef SECRETS_H
#define SECRETS_H

const char WIFI_SSID[] = "WIFI_SSID_HERE";
const char WIFI_PASSWORD[] = "WIFI_PASSWORD_HERE";

const char MYSQL_USER[] = "MYSQL_USER_HERE";
const char MYSQL_PASSWORD[] = "MYSQL_PASSWORD_HERE";
IPAddress MYSQL_SERVERIP(192, 168, 1, 100);         // MySQL server IP
const int MYSQL_SERVERPORT = 3306;                  // default 3306

#endif
