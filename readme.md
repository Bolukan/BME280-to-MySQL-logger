# BME280 to MySQL logger
Log sensordata (BME280) directly to MySQL database

This is a proof-of-concept for logging sensor data directly to a database without the use of MQTT, PHP/Python, etc., but with the use of a direct TCP-connection to the MySQL database.

## Requirements
* ESP8266 (D1 mini)  
An ESP8266 or ESP32  

* BME280 I2C  

* MySQL database  
Any MySQL database. My database is a MariaDB 10 on a Synology with port 3307 linked to /run/mysqld/mysqld10.sock  
I created a database `sensor`, a table `BME280` and an user with only 'Data' privileges for the `sensor` database.

```sql
CREATE TABLE `BME280` (
  `sensorid` int(11) DEFAULT NULL,
  `timestamp` timestamp NOT NULL DEFAULT current_timestamp(),
  `humidity` mediumint(6) NOT NULL,
  `pressure` mediumint(6) NOT NULL,
  `temperature` mediumint(6) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
```

## Instructions

* Prepare the database

* Connect a BME280 to an ESP8266 (D1 mini) using the standard I2C ports (D1=SCL, D2=SDA)

* Upload the program

## Feedback

Please send any comments and suggestions.
