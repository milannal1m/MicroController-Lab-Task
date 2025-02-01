# MicroController Lab Task

## Overview
This project uses an ESP32-C3 microcontroller to control a WS2812 LED strip and measure distances using an ultrasonic sensor. The brightness of the LEDs is adjusted dynamically based on an ADC reading. The LED colors change based on the measured distance, transitioning from green to red as an object approaches.

## Features
- **WS2812 LED Strip Control**: Displays colors dynamically based on distance measurements.
- **Ultrasonic Sensor Integration**: Measures distances and maps them to LED output.
- **ADC-Based Brightness Control**: Adjusts LED brightness based on an analog input.
- **Precise Timing with Assembly Code**: Uses inline assembly for accurate signal timing.
- **ESP32-C3 Low-Level Register Access**: Direct register manipulation for performance optimization.

## Hardware Requirements
- ESP32-C3 microcontroller
- WS2812 LED strip (16 LEDs)
- HC-SR04 Ultrasonic Distance Sensor
- Power supply (5V)
- Resistors and wiring as needed

## Software Requirements
- ESP-IDF (Espressif IoT Development Framework)
- C compiler compatible with ESP32-C3

## Wiring Diagram
| Component | ESP32-C3 Pin |
|-----------|-------------|
| WS2812 Data In | GPIO1 |
| HC-SR04 Trigger | GPIO2 |
| HC-SR04 Echo | GPIO3 |
| ADC Input | GPIO0 |

## Code Structure
- **`display()`**: Sends pixel data to the WS2812 LED strip.
- **`displayLEDs()`**: Updates the LED strip with new color values.
- **`config_timer()`**: Configures the ESP32 timer for precise measurements.
- **`readSensor()`**: Reads distance from the ultrasonic sensor.
- **`getDistance()`**: Computes the average of multiple sensor readings.
- **`setPixel()`**: Maps distance values to LED colors.
- **`adc_init()`**: Initializes the ADC for brightness control.
- **`adc_read()`**: Reads the ADC value.
- **`setBrightness()`**: Adjusts LED brightness based on ADC readings.
- **`loop()`**: Main program loop that updates LEDs based on sensor data.
- **`app_main()`**: Entry point of the program.

## Installation & Usage
1. Set up ESP-IDF on your development machine.
2. Clone this repository and navigate to the project directory.
3. Build and flash the firmware using:
   ```sh
   idf.py build flash monitor
   ```
4. Connect the ESP32-C3 to power and observe the LED behavior.

## Future Improvements
- Add WiFi/Bluetooth integration for remote control.
- Optimize ADC calibration for better brightness accuracy.
- Implement power-saving modes for efficiency.


