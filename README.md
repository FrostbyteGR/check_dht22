# check_dht22

## I. DESCRIPTION:

DHT22 Sensor nagios plugin for Single Board Computers

Copyright (c) 2017 Frostbyte <frostbytegr@gmail.com>

Check environment temperature and humidity with DHT22 via GPIO

## II. CREDITS:

* <projects@drogon.net> - wiringPi library and rht03 code
* <devel@nagios-plugins.org> - plugin development guidelines

## III. FEATURES:

* Output includes perfdata
* Threshold ranges (min/max) for both temperature and humidity
* Both warning and critical thresholds may be fully or partially omitted
  - Anything that has not been explicitly specified will be disabled
* Only allows for threshold ranges that are within the sensor's documented capabilities
  - Temperature: from -40 to 80
  - Humidity: from 0 to 100
* Output validation against the sensor's checksums and documented capabilities
  - If measured data are invalid, will retry every 4 seconds for a maximum of 2 times

## IV. SUPPORTED DEVICES:

* Raspberry Pi
* Raspberry Pi 2
* Raspberry Pi 3
* ASUS Tinker Board

## V. GPIO HEADER PINOUT

<img src="https://raw.githubusercontent.com/FrostbyteGR/check_dht22/master/Doc/j8header.png" width="380">

## VI. INSTALLATION:

1. Give execution permissions to the build script and execute it:
   - chmod u+x build && ./build
2. Copy the compiled plugin to the nagios plugins directory:
   - sudo cp bin/check_dht22 /usr/local/lib/nagios/plugins/check_dht22
3. Allow nagios/icinga to execute the plugin as sudo, by adding this entry to the sudoers file:
   - nagios ALL=(ALL) NOPASSWD: /usr/local/lib/nagios/plugins/check_dht22

## VII. USAGE:

* sudo check_dht22 -p <gpio_pin> [-w tmp_warn_range,hum_warn_range] [-c tmp_crit_range,hum_crit_range]
  - example: sudo check_dht22 -p 7 -w 10:40,30:70 -c 5:45,25:75
