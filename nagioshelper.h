/*
 * nagioshelper.h
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

// Sensor library
#include "dht22.h"

// Data structures
struct thresholdRange {
	int min;
	int max;
};

struct threshold {
	struct thresholdRange temperature;
	struct thresholdRange humidity;
};

struct execParameters {
	int GPIO;
	struct threshold warn;
	struct threshold crit;
};

// Function prototypes
struct execParameters parseParameters(int argc, char *argv[]);
int outputResults(struct execParameters params, struct sensorOutput output);
