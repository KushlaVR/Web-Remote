echo off
echo "Enter port number, and press Enter."

set /p portNumber="COM"

echo "Port number is COM%portNumber%"

esptool.exe --chip esp8266 --port COM%portNumber% write_flash 0 firmware-pio.bin

esptool.exe --chip esp8266 --port COM%portNumber% write_flash 0x00300000 spiffs-pio.bin

pause