# wio-terminal
several scripts for arduino wio-terminal with grove seeed sen55 air-quality monitor

1. Script to measure air-quality with a grove all-in-one environmental sensor sen55. Shows data on the LCD screen in colour. Main topic is particular dust PM2.5

2. Script to upload sensor data via MQTT, for instance for Home-assistant. That way Home-assistant can do smart things if air-quality is poor (like start ventilation or warn in some way)

3. Script that combines 1 + 2 so it measures air-quality via sen55 grove sensor and also posts this to MQTT
