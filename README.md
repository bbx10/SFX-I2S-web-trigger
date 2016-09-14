# SFX-I2S-web-trigger
ESP8266 Arduino Sound F/X I2S web trigger

![ESP8266, I2S DAC, speaker, battery](/images/sfx-i2s.png)

Press the 0 button to play file T0.wav, press the 1 button to play file T1.wav,
etc. The web interface is identical the related project
[SFX-web-trigger](https://github.com/bbx10/SFX-web-trigger). The other project
uses a much more expensive MP3/OGG decoder board. This project uses a $6 I2S
DAC with 3W audio amplifier board. The WAV audio files are stored in the ESP8266
Flash.

Most ESP-12s boards and modules come with 4 Mbytes of Flash of which 3 MBbytes is used as a Flash
file system named SPIFFS. To store the WAV files in the SPIFFS file system,
create a data directory in the same directory with the .INO file. Copy WAV
files to the directory with names T0.wav, T1.wav, ... T9.wav. Install the
[SPIFFS upload tool](http://www.esp8266.com/viewtopic.php?f=32&t=10081). Use
the tool to upload the WAV files to the ESP8266 Flash.

The WAV files should be mono (number of channels = 1) and contain uncompressed
16-bit PCM audio. Samples rates 11025, 22050, and 44100 have been verified to
work. The included test WAV files were generated using Festival (text to
speech) software.

Upload the ESPI2S.INO application. Connect any web browser to the ESP8266 web
server at http://espsfxtrigger.local. If this does not work use the ESP8266
IP address. This should bring up the web page shown above. Press buttons
0...9 to play WAV files T0.wav ... T9.wav.

If the ESP8266 is unable to connect to an Access Point, it becomes an Access
Point. Connect to it by going into your wireless settings. The AP name should
look like ESP\_SFX\_TRIGGER. Once connected the ESP will bring up a web page
where you can enter the SSID and password of your WiFi router.

How might this device be used? Load up your favorite sound samples such as
elephant trumpet, minion laugh, cat meow, etc. Bury the device in the couch
cushions. Wait for your victim to sit down. While you appear to play Candy
Crush on your phone, remotely trigger your favorite sound F/X!

## Hardware components

* [Adafruit I2S 3W Class D Amplifier Breakout - MAX98357A](https://www.adafruit.com/products/3006)
This board converts the digital audio data from the ESP8266 I2S controller to
analog audio and amplifies the signal to drive a speaker. This breakout board is
mono instead of stereo but it is only $6!

* [Adafruit Feather Huzzah with ESP8266](https://www.adafruit.com/products/2821)
This ESP8266 board was chosen because it has a lithium battery charger. Other
ESP8266 boards should also work.

* 3 Watt, 4 Ohm speaker

* Lithium battery

## Connection Diagram

Adafruit I2S DAC |ESP8266            | Description
-----------------|-------------------|-------------
LRC              |GPIO2/TX1 LRCK     | Left/Right audio
BCLK             |GPIO15 BCLK        | I2S Clock
DIN              |GPIO03/RX0 DATA    | I2S Data
GAIN             |not connected      | 9 dB gain
SD               |not connected      | Stereo average
GND              |GND                | Ground
Vin              |BAT                | 3.7V battery power

If you need more volume you could use a booster to generate 5V from the battery
and drive the DAC board Vin with 5V. Also experiment with the GAIN pin.

## Software components

* [WiFiManager](https://github.com/tzapu/WiFiManager) eliminates
the need to store the WiFi SSID and password in the source code. WiFiManager
will switch the ESP8266 to an access point and web server to get the SSID and
password when needed.

* [SPIFFS](http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html)
The SPIFFS Flash file system is included with the ESP8266 Arduino board support
package. The header file name is FS.h.

* ESP8266 I2S support is included with the ESP8266 Arduino board support package.
The header file names are i2s.h and i2s\_reg.h.

* [WebSockets](https://github.com/Links2004/arduinoWebSockets)

* Other components are included with the ESP8266 Arduino package.
