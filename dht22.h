/*
 * dht22.h
 *  DHT22 Sensor nagios plugin for Single Board Computers
 *  Copyright (c) 2017 Frostbyte <frostbytegr@gmail.com>
 *
 *  Check environment temperature and humidity with DHT22 via GPIO
 *
 *  Credits:
 *  <projects@drogon.net> - wiringPi library and rht03 code
 *  <devel@nagios-plugins.org> - plugin development guidelines
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Sensor definitions
#define SENSOR_NA 110
#define SENSOR_TMP_MIN -40
#define SENSOR_TMP_MAX 80
#define SENSOR_HUM_MIN 0
#define SENSOR_HUM_MAX 100

// Data structures
struct sensorOutput {
	float temperature;
	float humidity;
};

// Function prototypes
struct sensorOutput parseSensorOutput(int GPIO);
