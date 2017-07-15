/*
 * nagioshelper.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

// Helper library
#include "nagioshelper.h"

// Error code definitions
#define ERRCODE_USAGE				0
#define ERRCODE_INVALID_GPIO		1
#define ERRCODE_INVALID_THRESHOLD	2
#define ERRCODE_INVALID_TMP_RANGE	3
#define ERRCODE_INVALID_HUM_RANGE	4
#define ERRCODE_INVALID_TMP_RANGES	5
#define ERRCODE_INVALID_HUM_RANGES	6

// Disabled threshold range definitions
#define THRNG_DISABLE_MIN -110
#define THRNG_DISABLE_MAX 110

// Error handling function
static void throwError(int errorCode) {
	// Decide on which error to display
	switch(errorCode) {
		case ERRCODE_USAGE:
			fprintf(stderr, "Usage:\n" \
			"sudo check_dht22 -p <gpio_pin> [-w tmp_warn_range,hum_warn_range] [-c tmp_crit_range,hum_crit_range]\n" \
			"Example: sudo check_dht22 -p 7 -w 10:40,30:70 -c 5:45,25:75\n");
			break;
		case ERRCODE_INVALID_GPIO:
			fprintf(stderr, "Invalid GPIO pin specified.\n" \
			"Acceptable range: 0-31\n");
			break;
		case ERRCODE_INVALID_THRESHOLD:
			fprintf(stderr, "Invalid threshold range.\n" \
			"Acceptable formats: N:N, N:, :N, or N\n");
			break;
		case ERRCODE_INVALID_TMP_RANGE:
			fprintf(stderr, "Invalid temperature range.\n" \
			"Acceptable values: from %d to %d\n", SENSOR_TMP_MIN, SENSOR_TMP_MAX);
			break;
		case ERRCODE_INVALID_HUM_RANGE:
			fprintf(stderr, "Invalid humidity range.\n" \
			"Acceptable values: from %d to %d\n", SENSOR_HUM_MIN, SENSOR_HUM_MAX);
			break;
		case ERRCODE_INVALID_TMP_RANGES:
			fprintf(stderr, "The temperature warning threshold range must be a subset of the temperature critical threshold range.\n");
			break;
		case ERRCODE_INVALID_HUM_RANGES:
			fprintf(stderr, "The humidity warning threshold range must be a subset of the humidity critical threshold range.\n");
			break;
	}

	// Flush stderr and exit
	fflush(stderr);
	exit(EXIT_FAILURE);
}

// Default parameters generator function
static struct execParameters defaultParameters() {
	struct execParameters defaults;

	// Execution Parameter Defaults
	defaults.GPIO=-1;
	defaults.warn.temperature.min=THRNG_DISABLE_MIN;
	defaults.warn.temperature.max=THRNG_DISABLE_MAX;
	defaults.crit.temperature.min=THRNG_DISABLE_MIN;
	defaults.crit.temperature.max=THRNG_DISABLE_MAX;
	defaults.warn.humidity.min=THRNG_DISABLE_MIN;
	defaults.warn.humidity.max=THRNG_DISABLE_MAX;
	defaults.crit.humidity.min=THRNG_DISABLE_MIN;
	defaults.crit.humidity.max=THRNG_DISABLE_MAX;

	return defaults;
}

// Normalize function for user input: Threshold Ranges
static struct execParameters normalizeThresholdRanges(struct execParameters params) {
	struct execParameters result=params;

	// If no temperature warning minimum was supplied but a temperature critical minimum was
	if (result.warn.temperature.min==THRNG_DISABLE_MIN && result.crit.temperature.min!=THRNG_DISABLE_MAX) {
		// Set the temperature warning minimum equal to the temperature critical minimum
		result.warn.temperature.min=result.crit.temperature.min;
	}

	// If no temperature warning maximum was supplied but a temperature critical maximum was
	if (result.warn.temperature.max==THRNG_DISABLE_MAX && result.crit.temperature.max!=THRNG_DISABLE_MAX) {
		// Set the temperature warning maximum equal to the temperature critical maximum
		result.warn.temperature.max=result.crit.temperature.max;
	}

	// If no humidity warning minimum was supplied but a humidity critical minimum was
	if (result.warn.humidity.min==THRNG_DISABLE_MIN && result.crit.humidity.min!=THRNG_DISABLE_MIN) {
		// Set the humidity warning minimum equal to the humidity critical minimum
		result.warn.humidity.min=result.crit.humidity.min;
	}

	// If no humidity warning maximum was supplied but a humidity critical maximum was
	if (result.warn.humidity.max==THRNG_DISABLE_MAX && result.crit.humidity.max!=THRNG_DISABLE_MAX) {
		// Set the humidity warning maximum equal to the humidity critical maximum
		result.warn.humidity.max=result.crit.humidity.max;
	}

	// Return the processed parameters
	return result;
}

// Validation function for user input: Threshold Range
static int validateThresholdRange(char *inputString) {
	// Convert input to integer
	int result=atoi(inputString);

	// If the input starts with a negative sign
	if (*inputString=='-') {
		// Advance the pointer to the next character
		inputString++;
	}

	// Check input for any non-numerical characters
	while (*inputString) {
		if (isdigit(*inputString++)==0) {
			// If a non-numerical character is found, throw the corresponding error
			throwError(ERRCODE_INVALID_THRESHOLD);
		}
	}

	// Return the processed range
	return result;
}

// Validation function for user input: Threshold Ranges
static struct execParameters validateThresholdRanges(struct execParameters params) {
	// Divide the ranges into minimums and maximums
	int tmpMinRanges[]={params.warn.temperature.min, params.crit.temperature.min};
	int tmpMaxRanges[]={params.warn.temperature.max, params.crit.temperature.max};
	int humMinRanges[]={params.warn.humidity.min, params.crit.humidity.min};
	int humMaxRanges[]={params.warn.humidity.max, params.crit.humidity.max};

	// Loop through the ranges
	for (int range=0; range<2; range++) {
		// If any of the temperature ranges are not within the sensor's documented capabilities
		if (((tmpMinRanges[range]<SENSOR_TMP_MIN || tmpMinRanges[range]>SENSOR_TMP_MAX) && tmpMinRanges[range]!=THRNG_DISABLE_MIN) \
		|| ((tmpMaxRanges[range]<SENSOR_TMP_MIN || tmpMaxRanges[range]>SENSOR_TMP_MAX) && tmpMaxRanges[range]!=THRNG_DISABLE_MAX)) {
			// Throw the corresponding error
			throwError(ERRCODE_INVALID_TMP_RANGE);
		}

		// If any of the humidity ranges are not within the sensor's documented capabilities
		if (((humMinRanges[range]<SENSOR_HUM_MIN || humMinRanges[range]>SENSOR_HUM_MAX) && humMinRanges[range]!=THRNG_DISABLE_MIN) \
		|| ((humMaxRanges[range]<SENSOR_HUM_MIN || humMaxRanges[range]>SENSOR_HUM_MAX) && humMaxRanges[range]!=THRNG_DISABLE_MAX)) {
			// Throw the corresponding error
			throwError(ERRCODE_INVALID_HUM_RANGE);
		}
	}

	// Normalize the ranges, if needed
	struct execParameters result=normalizeThresholdRanges(params);

	// If the temperature warning threshold range is larger than the temperature critical threshold range
	if (result.warn.temperature.min<result.crit.temperature.min || result.warn.temperature.max>result.crit.temperature.max) {
		// Throw the corresponding error
		throwError(ERRCODE_INVALID_TMP_RANGES);
	}

	// If the humidity warning threshold range is larger than the humidity critical threshold range
	if (result.warn.humidity.min<result.crit.humidity.min || result.warn.humidity.max>result.crit.humidity.max) {
		// Throw the corresponding error
		throwError(ERRCODE_INVALID_HUM_RANGES);
	}

	// Return the processed parameters
	return result;
}

// Parser function for user input: GPIO
static int parseGPIO(char *inputString) {
	// Convert input to integer
	int result=atoi(inputString);

	// Check input for any non-numerical characters
	while (*inputString) {
		if (isdigit(*inputString++)==0) {
			// If a non-numerical character is found, throw the corresponding error
			throwError(ERRCODE_INVALID_GPIO);
		}
	}

	// If the supplied GPIO pin is not within the acceptable range
	if (result<0 || result>31) {
		// Throw the corresponding error
		throwError(ERRCODE_INVALID_GPIO);
	}

	// Return the processed GPIO
	return result;
}

// Parser function for user input: Threshold Range
static struct thresholdRange parseThresholdRange(char *inputString) {
	struct thresholdRange result;
	char *delimiter;

	// If no delimiter was found within the input string
	if ((delimiter=strchr(inputString, ':'))==NULL) {
		// Set the threshold range maximum and disable the threshold range minimum
		result.max=validateThresholdRange(inputString);
		result.min=THRNG_DISABLE_MIN;

		// Return the processed threshold range
		return result;
	} else {
		// If the delimiter was found at the start of the input string
		if (*inputString==':') {
			// Set the threshold range maximum and disable the threshold range minimum
			result.max=validateThresholdRange(inputString+1);
			result.min=THRNG_DISABLE_MIN;

			// Return the processed threshold range
			return result;
		}

		// If the delimiter was found at the end of the input string
		if (*(delimiter+1)=='\0') {
			// Eliminate the delimiter so it can pass validation
			*delimiter='\0';

			// Set the threshold range minimum and disable the threshold range maximum
			result.min=validateThresholdRange(inputString);
			result.max=THRNG_DISABLE_MAX;

			// Return the processed threshold range
			return result;
		}
	}

	// If the delimiter was found, and is neither at the start nor at the end of the input string

	// Eliminate the delimiter so it can pass validation
	*delimiter='\0';

	// Set the threshold range minimum and maximum
	result.min=validateThresholdRange(inputString);
	result.max=validateThresholdRange(delimiter+1);

	// If the threshold range minimum is greater than the threshold range maximum
	if (result.min>result.max) {
		// Swap the values around
		int swapHelper=result.min;
		result.min=result.max;
		result.max=swapHelper;
	}

	// Return the processed threshold range
	return result;
}

// Parser function for user input: Threshold
static struct threshold parseThreshold(char *inputString) {
	struct threshold result;
	char *delimiter;

	// If no delimiter was found within the input string, or was found at the start or the end of the input string
	if ((delimiter=strchr(inputString, ','))==NULL) {
		// Set the temperature threshold and disable the humidity threshold
		result.temperature=parseThresholdRange(inputString);
		result.humidity.min=THRNG_DISABLE_MIN;
		result.humidity.max=THRNG_DISABLE_MAX;

		// Return the processed thresholds
		return result;
	} else {
		// If the delimiter was found at the start of the input string
		if (*inputString==','){
			// Set the humidity threshold and disable the temperature threshold
			result.humidity=parseThresholdRange(inputString+1);
			result.temperature.min=THRNG_DISABLE_MIN;
			result.temperature.max=THRNG_DISABLE_MAX;

			// Return the processed thresholds
			return result;
		}

		// If the delimiter was found at the end of the input string
		if (*(delimiter+1)=='\0'){
			// Eliminate the delimiter so it can pass validation
			*delimiter='\0';

			// Set the temperature threshold and disable the humidity threshold
			result.temperature=parseThresholdRange(inputString);
			result.humidity.min=THRNG_DISABLE_MIN;
			result.humidity.max=THRNG_DISABLE_MAX;

			// Return the processed thresholds
			return result;
		}
	}

	// If the delimiter was found, and is neither at the start nor at the end of the input string

	// Eliminate the delimiter so it can pass validation
	*delimiter='\0';

	// Parse and return the processed thresholds
	result.temperature=parseThresholdRange(inputString);
	result.humidity=parseThresholdRange(delimiter+1);
	return result;
}

// Parser function for user input: Execution Parameters
struct execParameters parseParameters(int argc, char *argv[]) {
	int argument;

	// Set the execution parameter defaults
	struct execParameters result=defaultParameters();

	// Process the user input
	while ((argument=getopt(argc, argv, "p:w:c:"))!=-1) {
		switch (argument) {
			case 'p':
				result.GPIO=parseGPIO(optarg);
				break;
			case 'w':
				result.warn=parseThreshold(optarg);
				break;
			case 'c':
				result.crit=parseThreshold(optarg);
				break;
			default:
				throwError(ERRCODE_USAGE);
		}
	}

	// If the user did not supply a GPIO pin
	if (result.GPIO==-1) {
		// Respond with the usage error
		throwError(ERRCODE_USAGE);
	}

	// Ensure the threshold ranges are correct
	result=validateThresholdRanges(result);

	// Return the processed execution parameters
	return result;
}

// Standard nagios response function
int outputResults(struct execParameters params, struct sensorOutput output) {
	// Declaration of possible nagios check states
	const char *states[4]={"OK","WARNING","CRITICAL","UNKNOWN"};

	// Set initial status to UNKNOWN
	int result=3;

	// If the sensor output contains valid values
	if (output.temperature!=SENSOR_NA && output.humidity!=SENSOR_NA) {
		// If the reported temperature and humidity are within the warning range
		if (output.temperature>=params.warn.temperature.min && output.temperature<=params.warn.temperature.max && output.humidity>=params.warn.humidity.min && output.humidity<=params.warn.humidity.max) {
			// Set status to OK
			result=0;
		} else {
			// If the reported temperature and humidity are within the critical range
			if (output.temperature>=params.crit.temperature.min && output.temperature<=params.crit.temperature.max && output.humidity>=params.crit.humidity.min && output.humidity<=params.crit.humidity.max) {
				// Set status to WARNING
				result=1;
			} else {
				// Set status to CRITICAL
				result=2;
			}
		}
	} else {
		// If the sensor output doesn't contain valid values
		// Set them to zero so the output is normalized
		output.temperature=0;
		output.humidity=0;
	}

	// Issue the final response to the user
	fprintf(stdout, "%s - Temperature: %.1fC Humidity: %.1f%% | tmp=%.1f;%d;%d;0 hum=%.1f;%d;%d;0\n", states[result], output.temperature, output.humidity, output.temperature, params.warn.temperature.max, params.crit.temperature.max, output.humidity, params.warn.humidity.max, params.crit.humidity.max);
	fflush(stdout);
	return result;
}
