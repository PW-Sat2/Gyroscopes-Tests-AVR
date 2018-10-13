# Gyroscopes-Tests-AVR
Embedded software for gyroscopes tests board.

Gyroscope models:
* InvenSense ITG3200
* ST A3G4250D

The application supports PCF8563 RTC and SD cards with FAT32 file system.

# About tests

The main goals of performed tests were to check performance of ITG3200 and A3G4250D gyroscopes in terms of static, dynamic and thermal parameters and eventually pick the one that fulfils requirements.

![dsc_0015](https://user-images.githubusercontent.com/6267528/46909613-75b7db80-cf35-11e8-8425-439875f1f9fb.JPG)

A board equipped with SD card (FAT FS support) and real-time clock. Live data feed via UART is also possible.

Dynamic tests were performed on IXMOTION EVO-20 positioning and rate table in Rzeszów. Angular rates for axis-1 is 0 – 1500 deg/sec, axis-2 is 0 – 600 deg/sec. In both axes resolution is 0.1 deg/sec.

# Test conclusions

Although A3G4250D gyroscopes offers higher resolution, their noise performance is very poor. ITG3200 sensors offer satisfactory resolution and noise performance and are able to detect angular rates as low as 0.1 degrees per second. Thermal dependence of null output for A3G4250D is more predictable and linear, whereas ITG3200 seems to exhibit some hysteresis, but further studies are needed to confirm this phenomenon. Built-in temperature sensor in ITG3200 is accurate enough for thermal compensation purposes, while temperature sensor in A3G4250D seems to be unusable.
What is more, significant problems (random ‘spikes’), probably with 16-bit data inconsistency stored in two 8-bit registers, were noticed in the case of A3G4250D.
Taking into account pros and cons of both sensors, it is recommended to use ITG3200 from InvenSense (https://www.invensense.com/products/motion-tracking/3-axis/itg-3200/).
