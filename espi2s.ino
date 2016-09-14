/*****************************************************************************
  The MIT License (MIT)

  Copyright (c) 2016 by bbx10node@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 **************************************************************************/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>   // https://github.com/Links2004/arduinoWebSockets
#include <Hash.h>

#include <i2s.h>
#include <i2s_reg.h>
#include "wavspiffs.h"

// Non-blocking I2S write for left and right 16-bit PCM
bool ICACHE_FLASH_ATTR i2s_write_lr_nb(int16_t left, int16_t right){
  int sample = right & 0xFFFF;
  sample = sample << 16;
  sample |= left & 0xFFFF;
  return i2s_write_sample_nb(sample);
}

struct I2S_status_s {
  wavFILE_t wf;
  int16_t buffer[512];
  int bufferlen;
  int buffer_index;
  int playing;
} I2S_WAV;

void wav_stopPlaying()
{
  i2s_end();
  I2S_WAV.playing = false;
  wavClose(&I2S_WAV.wf);
}

bool wav_playing()
{
  return I2S_WAV.playing;
}

void wav_setup()
{
  Serial.println(F("wav_setup"));
  I2S_WAV.bufferlen = -1;
  I2S_WAV.buffer_index = 0;
  I2S_WAV.playing = false;
}

void wav_loop()
{
  bool i2s_full = false;
  int rc;

  while (I2S_WAV.playing && !i2s_full) {
    while (I2S_WAV.buffer_index < I2S_WAV.bufferlen) {
      int16_t pcm = I2S_WAV.buffer[I2S_WAV.buffer_index];
      if (i2s_write_lr_nb(pcm, pcm)) {
        I2S_WAV.buffer_index++;
      }
      else {
        i2s_full = true;
        break;
      }
      if ((I2S_WAV.buffer_index & 0x3F) == 0) yield();
    }
    if (i2s_full) break;

    rc = wavRead(&I2S_WAV.wf, I2S_WAV.buffer, sizeof(I2S_WAV.buffer));
    if (rc > 0) {
      //Serial.printf("wavRead %d\r\n", rc);
      I2S_WAV.bufferlen = rc / sizeof(I2S_WAV.buffer[0]);
      I2S_WAV.buffer_index = 0;
    }
    else {
      Serial.println(F("Stop playing"));
      wav_stopPlaying();
      break;
    }
  }
}

void wav_startPlayingFile(const char *wavfilename)
{
  wavProperties_t wProps;
  int rc;

  Serial.printf("wav_starPlayingFile(%s)\r\n", wavfilename);
  i2s_begin();
  rc = wavOpen(wavfilename, &I2S_WAV.wf, &wProps);
  Serial.printf("wavOpen %d\r\n", rc);
  if (rc != 0) {
    Serial.println("wavOpen failed");
    return;
  }
  Serial.printf("audioFormat %d\r\n", wProps.audioFormat);
  Serial.printf("numChannels %d\r\n", wProps.numChannels);
  Serial.printf("sampleRate %d\r\n", wProps.sampleRate);
  Serial.printf("byteRate %d\r\n", wProps.byteRate);
  Serial.printf("blockAlign %d\r\n", wProps.blockAlign);
  Serial.printf("bitsPerSample %d\r\n", wProps.bitsPerSample);

  i2s_set_rate(wProps.sampleRate);

  I2S_WAV.bufferlen = -1;
  I2S_WAV.buffer_index = 0;
  I2S_WAV.playing = true;
  wav_loop();
}

/**************************************************************************************/

/*
 * index.html
 */
// This string holds HTML, CSS, and Javascript for the HTML5 UI.
// The browser must support HTML5 WebSockets which is true for all modern browsers.
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>ESP8266 Sound Effects Web Trigger</title>
<style>
body { font-family: Arial, Helvetica, Sans-Serif; }
table { border: 1px solid black; }
th { border: 1px solid black; }
td { text-align: left; border: 1px solid black; }
.SFXtd { text-align: right; }
.SFXButton { font-size: 125%; background-color: #E0E0E0; border: 1px solid; }
</style>
<script>
function enableTouch(objname) {
  console.log('enableTouch', objname);
  var e = document.getElementById(objname);
  if (e) {
    e.addEventListener('touchstart', function(event) {
        event.preventDefault();
        console.log('touchstart', event);
        buttondown(e);
        }, false );
    e.addEventListener('touchend',   function(event) {
        console.log('touchend', event);
        buttonup(e);
        }, false );
  }
  else {
    console.log(objname, ' not found');
  }
}

var websock;
var WebSockOpen=0;  //0=close,1=opening,2=open

function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  WebSockOpen=1;
  websock.onopen = function(evt) {
    console.log('websock open');
    WebSockOpen=2;
    var e = document.getElementById('webSockStatus');
    e.style.backgroundColor = 'green';
  };
  websock.onclose = function(evt) {
    console.log('websock close');
    WebSockOpen=0;
    var e = document.getElementById('webSockStatus');
    e.style.backgroundColor = 'red';
  };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    var nowPlaying = document.getElementById('nowPlaying');
    if (evt.data.startsWith('nowPlaying=')) {
      nowPlaying.innerHTML = evt.data;
    }
    else {
      console.log('unknown event', evt.data);
    }
  };

  var allButtons = [
    'bSFX0',
    'bSFX1',
    'bSFX2',
    'bSFX3',
    'bSFX4',
    'bSFX5',
    'bSFX6',
    'bSFX7',
    'bSFX8',
    'bSFX9',
  ];
  for (var i = 0; i < allButtons.length; i++) {
    enableTouch(allButtons[i]);
  }
}

function buttondown(e) {
  switch (WebSockOpen) {
    case 0:
      window.location.reload();
      WebSockOpen=1;
      break;
    case 1:
    default:
      break;
    case 2:
      websock.send(e.id + '=1');
      break;
  }
}

function buttonup(e) {
  switch (WebSockOpen) {
    case 0:
      window.location.reload();
      WebSockOpen=1;
      break;
    case 1:
    default:
      break;
    case 2:
      websock.send(e.id + '=0');
      break;
  }
}
</script>
</head>
<body onload="javascript:start();">
<h2>ESP SF/X Web Trigger</h2>
<div id="nowPlaying">Now Playing</div>
<table>
  <tr>
    <td class="SFXtd"><button id="bSFX0" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">0</button></td>
    <td>Track 0</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX1" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">1</button></td>
    <td>Track 1</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX2" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">2</button></td>
    <td>Track 2</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX3" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">3</button></td>
    <td>Track 3</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX4" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">4</button></td>
    <td>Track 4</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX5" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">5</button></td>
    <td>Track 5</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX6" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">6</button></td>
    <td>Track 6</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX7" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">7</button></td>
    <td>Track 7</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX8" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">8</button></td>
    <td>Track 8</td>
  </tr>
  <tr>
    <td class="SFXtd"><button id="bSFX9" type="button" class="SFXButton"
      onmousedown="buttondown(this);" onmouseup="buttonup(this);">9</button></td>
    <td>Track 9</td>
  </tr>
</table>
<p>
</body>
</html>
)rawliteral";

/*
 * Web server and websocket server
 */
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void startPlaying(const char *filename)
{
  char nowPlaying[80] = "nowPlaying=";

  wav_startPlayingFile(filename);
  strncat(nowPlaying, filename, sizeof(nowPlaying)-strlen(nowPlaying)-1);
  webSocket.broadcastTXT(nowPlaying);
}

void update_browser() {
  if (!wav_playing()) {
    webSocket.broadcastTXT("nowPlaying=");
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      // Looks for button press "bSFXn=1" messages where n='0'..'9'
      if ((length == 7) &&
          (memcmp((const char *)payload, "bSFX", 4) == 0) &&
          (payload[6] == '1')) {
        switch (payload[4]) {
          case '0':
            startPlaying("/T0.wav");
            break;
          case '1':
            startPlaying("/T1.wav");
            break;
          case '2':
            startPlaying("/T2.wav");
            break;
          case '3':
            startPlaying("/T3.wav");
            break;
          case '4':
            startPlaying("/T4.wav");
            break;
          case '5':
            startPlaying("/T5.wav");
            break;
          case '6':
            startPlaying("/T6.wav");
            break;
          case '7':
            startPlaying("/T7.wav");
            break;
          case '8':
            startPlaying("/T8.wav");
            break;
          case '9':
            startPlaying("/T9.wav");
            break;
        }
      }
      else {
        Serial.printf("Unknown message from client [%s]\r\n", payload);
      }

      // send message to client
      // webSocket.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocket.broadcastTXT("message here");
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }
}

void webserver_setup(void)
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("websocket server started");

  if(MDNS.begin("espsfxtrigger")) {
    Serial.println("MDNS responder started. Connect to http://espsfxtrigger.local/");
  }
  else {
    Serial.println("MDNS responder failed");
  }

  // handle "/"
  server.on("/", []() {
      server.send_P(200, "text/html", INDEX_HTML);
  });

  server.begin();
  Serial.println("web server started");

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
}

inline void webserver_loop(void) {
  webSocket.loop();
  server.handleClient();
  update_browser();
}

/**************************************************************************************/

/*
 * WiFiManager (https://github.com/tzapu/WiFiManager)
 */
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager

void wifiman_setup(void)
{
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings SSID and password
  //wifiManager.resetSettings();

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("ESP_SFX_Trigger");

  //if you get here you have connected to the WiFi
  Serial.println(F("connected"));
}

void showDir(void)
{
  wavFILE_t wFile;
  wavProperties_t wProps;
  int rc;

  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    Serial.println(dir.fileName());
    rc = wavOpen(dir.fileName().c_str(), &wFile, &wProps);
    if (rc == 0) {
      Serial.printf("  audioFormat %d\r\n", wProps.audioFormat);
      Serial.printf("  numChannels %d\r\n", wProps.numChannels);
      Serial.printf("  sampleRate %d\r\n", wProps.sampleRate);
      Serial.printf("  byteRate %d\r\n", wProps.byteRate);
      Serial.printf("  blockAlign %d\r\n", wProps.blockAlign);
      Serial.printf("  bitsPerSample %d\r\n", wProps.bitsPerSample);
      Serial.println();
      wavClose(&wFile);
    }
  }
}

void setup()
{
  Serial.begin(115200); Serial.println();
  Serial.println(F("\nESP8266 Sound Effects Web Trigger"));

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS.begin() failed");
    return;
  }
  // Confirm track files are present in SPIFFS
  showDir();

  wifiman_setup();
  wav_setup();
  webserver_setup();
}

void loop()
{
  wav_loop();
  webserver_loop();
}
