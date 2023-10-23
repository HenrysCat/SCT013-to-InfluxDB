# SCT013-to-InfluxDB
Send readings from an SCT013 non invasive Current transformer to InfluxDB using an esp8266 D1 Mini

This example uses a 30A 1V version and assumes 240v supply.

It uses a Mains Power Sensor board from https://www.mottramlabs.com/esp_products.html

It also has a basic web interface that displays the RMS Current and power in Watts, the ip address will be printed in the serial monitor when it connects to WiFi along with ADC Reading, RMS Voltage, Actual Current and Power in Watts


Any errors relating to your influxdb connection are also printed to help with connection issues.
