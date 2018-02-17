// ESP8266 MIDI Gateway between Serial MIDI <-> WiFi AppleMIDI

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266LLMNR.h>
#include <ESP8266HTTPUpdateServer.h>
#include <MIDI.h>
#include <AppleMidi.h>

#define PEDALINO_DEBUG

#define WIFI_CONNECT_TIMEOUT    30
#define SMART_CONFIG_TIMEOUT    60

const char host[]     = "pedalino";

ESP8266WebServer        httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


// WiFi MIDI interface to comunicate with AppleMIDI/RTP-MDI devices

APPLEMIDI_CREATE_INSTANCE(WiFiUDP, AppleMIDI); // see definition in AppleMidi_Defs.h


// Serial MIDI interface to comunicate with Arduino

struct SerialMIDISettings : public midi::DefaultSettings
{
  static const long BaudRate = 115200;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, SerialMIDISettings);


// Forward messages received from serial MIDI interface to WiFI MIDI interface

void handleNoteOn(byte channel, byte note, byte velocity)
{
  AppleMIDI.noteOn(note, velocity, channel);
}

void handleNoteOff(byte channel, byte note, byte velocity)
{
  AppleMIDI.noteOff(note, velocity, channel);
}

void handleAfterTouchPoly(byte channel, byte note, byte pressure)
{
  AppleMIDI.polyPressure(note, pressure, channel);
}

void handleControlChange(byte channel, byte number, byte value)
{
  AppleMIDI.controlChange(number, value, channel);
}

void handleProgramChange(byte channel, byte number)
{
  AppleMIDI.programChange(number, channel);
}

void handleAfterTouchChannel(byte channel, byte pressure)
{
  AppleMIDI.afterTouch(pressure, channel);
}

void handlePitchBend(byte channel, int bend)
{
  AppleMIDI.pitchBend(bend, channel);
}

void handleSystemExclusive(byte* array, unsigned size)
{
  AppleMIDI.sysEx(array, size);
}

void handleTimeCodeQuarterFrame(byte data)
{
  AppleMIDI.timeCodeQuarterFrame(data);
}

void handleSongPosition(unsigned int beats)
{
  AppleMIDI.songPosition(beats);
}

void handleSongSelect(byte songnumber)
{
  AppleMIDI.songSelect(songnumber);
}

void handleTuneRequest(void)
{
  AppleMIDI.tuneRequest();
}

void handleClock(void)
{
  AppleMIDI.clock();
}

void handleStart(void)
{
  AppleMIDI.start();
}

void handleContinue(void)
{
  AppleMIDI._continue();
}

void handleStop(void)
{
  AppleMIDI.stop();
}

void handleActiveSensing(void)
{
  AppleMIDI.activeSensing();
}

void handleSystemReset(void)
{
  AppleMIDI.reset();
}


// Forward messages received from WiFI MIDI interface to serial MIDI interface

void OnAppleMidiConnected(uint32_t ssrc, char* name)
{
  //isConnected  = true;
}

void OnAppleMidiDisconnected(uint32_t ssrc)
{
  //isConnected  = false;
}

void OnAppleMidiNoteOn(byte channel, byte note, byte velocity)
{
  MIDI.sendNoteOn(note, velocity, channel);
}

void OnAppleMidiNoteOff(byte channel, byte note, byte velocity)
{
  MIDI.sendNoteOff(note, velocity, channel);
}

void OnAppleMidiReceiveAfterTouchPoly(byte channel, byte note, byte pressure)
{
  MIDI.sendPolyPressure(note, pressure, channel);
}

void OnAppleMidiReceiveControlChange(byte channel, byte number, byte value)
{
  MIDI.sendControlChange(number, value, channel);
}

void OnAppleMidiReceiveProgramChange(byte channel, byte number)
{
  MIDI.sendProgramChange(number, channel);
}
void OnAppleMidiReceiveAfterTouchChannel(byte channel, byte pressure)
{
  MIDI.sendAfterTouch(pressure, channel);
}

void OnAppleMidiReceivePitchBend(byte channel, int bend)
{
  MIDI.sendPitchBend(bend, channel);
}

void OnAppleMidiReceiveSysEx(const byte * data, uint16_t size)
{
  MIDI.sendSysEx(size, data);
}

void OnAppleMidiReceiveTimeCodeQuarterFrame(byte data)
{
  MIDI.sendTimeCodeQuarterFrame(data);
}

void OnAppleMidiReceiveSongPosition(unsigned short beats)
{
  MIDI.sendSongPosition(beats);
}

void OnAppleMidiReceiveSongSelect(byte songnumber)
{
  MIDI.sendSongSelect(songnumber);
}

void OnAppleMidiReceiveTuneRequest(void)
{
  MIDI.sendTuneRequest();
}

void OnAppleMidiReceiveClock(void)
{
  MIDI.sendRealTime(midi::Clock);
}

void OnAppleMidiReceiveStart(void)
{
  MIDI.sendRealTime(midi::Start);
}

void OnAppleMidiReceiveContinue(void)
{
  MIDI.sendRealTime(midi::Continue);
}

void OnAppleMidiReceiveStop(void)
{
  MIDI.sendRealTime(midi::Stop);
}

void OnAppleMidiReceiveActiveSensing(void)
{
  MIDI.sendRealTime(midi::ActiveSensing);
}

void OnAppleMidiReceiveReset(void)
{
  MIDI.sendRealTime(midi::SystemReset);
}


void ap_mode_start()
{
  WiFi.mode(WIFI_AP);
  boolean result = WiFi.softAP("Pedalino");

  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
}


bool smart_config()
{
  // Return 'true' if SSID and password received within SMART_CONFIG_TIMEOUT seconds

  // Re-establish lost connection to the AP
  WiFi.setAutoReconnect(true);

  // Automatically connect on power on to the last used access point
  WiFi.setAutoConnect(true);

  // Waiting for SSID and password from from mobile app
  // SmartConfig works only in STA mode
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();

#ifdef PEDALINO_DEBUG
  Serial.println("");
  Serial.print("SmartConfig started");
#endif

  for (int i = 0; i < SMART_CONFIG_TIMEOUT * 2 && !WiFi.smartConfigDone(); i++) {
    delay(500);
#ifdef PEDALINO_DEBUG
    Serial.print(".");
#endif
  }

#ifdef PEDALINO_DEBUG
  if (WiFi.smartConfigDone())
  {
    Serial.println("[SUCCESS]");
    Serial.printf("SSID        : %s\n", WiFi.SSID().c_str());
    Serial.printf("Password    : %s\n", WiFi.psk().c_str());
  }
  else
    Serial.println("[NO SUCCESS]");
#endif

  WiFi.stopSmartConfig();

  return WiFi.stopSmartConfig();
}

bool auto_reconnect()
{
  // Return 'true' if connected to the (last used) access point within WIFI_CONNECT_TIMEOUT seconds

#ifdef PEDALINO_DEBUG
  Serial.println("");
  Serial.printf("Connecting to %s ", WiFi.SSID().c_str());
#endif

  for (byte i = 0; i < WIFI_CONNECT_TIMEOUT * 2 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
#ifdef PEDALINO_DEBUG
    Serial.print(".");
#endif
  }

#ifdef PEDALINO_DEBUG
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("[SUCCESS]");
  else
    Serial.println("[NO SUCCESS]");
#endif

  return WiFi.status() == WL_CONNECTED;
}

void wifi_connect()
{
  if (!auto_reconnect())       // WIFI_CONNECT_TIMEOUT seconds to reconnect to last used access point
    if (smart_config())        // SMART_CONFIG_TIMEOUT seconds to receive SmartConfig parameters
      auto_reconnect();        // WIFI_CONNECT_TIMEOUT seconds to connect to SmartConfig access point
  if (WiFi.status() != WL_CONNECTED)
    ap_mode_start();          // switch to AP mode until next reboot
  else
  {
    WiFi.hostname(host);

#ifdef PEDALINO_DEBUG
    Serial.println("");
    WiFi.printDiag(Serial);
    Serial.println("");

    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    Serial.printf("BSSID       : %s\n", WiFi.BSSIDstr().c_str());
    Serial.printf("RSSI        : %d dBm\n", WiFi.RSSI());
    Serial.printf("Hostname    : %s\n", WiFi.hostname().c_str());
    Serial.printf("STA         : %02X:%02X:%02X:%02X:%02X:%02X\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    Serial.printf("IP address  : %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Subnet mask : %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("Gataway IP  : %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS 1       : %s\n", WiFi.dnsIP(0).toString().c_str());
    Serial.printf("DNS 2       : %s\n", WiFi.dnsIP(1).toString().c_str());
    Serial.println("");
#endif
    // Start LLMNR (Link-Local Multicast Name Resolution) responder
    LLMNR.begin(host);
#ifdef PEDALINO_DEBUG
    Serial.println("LLMNR responder started");
#endif

    // Start mDNS (Multicast DNS) responder
    if (MDNS.begin(host, WiFi.localIP())) {
#ifdef PEDALINO_DEBUG
      Serial.println("mDNS responder started");
#endif
      MDNS.addService("apple-midi", "udp", 5004);
    }
    // Start firmawre update via HTTP (connect to http://pedalino/update)
    httpUpdater.setup(&httpServer);
    httpServer.begin();
    MDNS.addService("http", "tcp", 80);
#ifdef PEDALINO_DEBUG
    Serial.println("HTTP server started");
    Serial.println("Connect to http://pedalino/update for firmware update");
#endif
  }

}

void midi_connect()
{
  // Connect the handle function called upon reception of a MIDI message from serial MIDI interface
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleAfterTouchPoly(handleAfterTouchPoly);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandleAfterTouchChannel(handleAfterTouchChannel);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleSystemExclusive(handleSystemExclusive);
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  MIDI.setHandleSongPosition(handleSongPosition);
  MIDI.setHandleSongSelect(handleSongSelect);
  MIDI.setHandleTuneRequest(handleTuneRequest);
  MIDI.setHandleClock(handleClock);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleContinue(handleContinue);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleActiveSensing(handleActiveSensing);
  MIDI.setHandleSystemReset(handleSystemReset);

  // Initiate serial MIDI communications, listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Create a session and wait for a remote host to connect to us
  AppleMIDI.begin("Pedalino(TM)");

  AppleMIDI.OnConnected(OnAppleMidiConnected);
  AppleMIDI.OnDisconnected(OnAppleMidiDisconnected);

  // Connect the handle function called upon reception of a MIDI message from WiFi MIDI interface
  AppleMIDI.OnReceiveNoteOn(OnAppleMidiNoteOn);
  AppleMIDI.OnReceiveNoteOff(OnAppleMidiNoteOff);
  AppleMIDI.OnReceiveAfterTouchPoly(OnAppleMidiReceiveAfterTouchPoly);
  AppleMIDI.OnReceiveControlChange(OnAppleMidiReceiveControlChange);
  AppleMIDI.OnReceiveProgramChange(OnAppleMidiReceiveProgramChange);
  AppleMIDI.OnReceiveAfterTouchChannel(OnAppleMidiReceiveAfterTouchChannel);
  AppleMIDI.OnReceivePitchBend(OnAppleMidiReceivePitchBend);
  AppleMIDI.OnReceiveSysEx(OnAppleMidiReceiveSysEx);
  AppleMIDI.OnReceiveTimeCodeQuarterFrame(OnAppleMidiReceiveTimeCodeQuarterFrame);
  AppleMIDI.OnReceiveSongPosition(OnAppleMidiReceiveSongPosition);
  AppleMIDI.OnReceiveSongSelect(OnAppleMidiReceiveSongSelect);
  AppleMIDI.OnReceiveTuneRequest(OnAppleMidiReceiveTuneRequest);
  AppleMIDI.OnReceiveClock(OnAppleMidiReceiveClock);
  AppleMIDI.OnReceiveStart(OnAppleMidiReceiveStart);
  AppleMIDI.OnReceiveContinue(OnAppleMidiReceiveContinue);
  AppleMIDI.OnReceiveStop(OnAppleMidiReceiveStop);
  AppleMIDI.OnReceiveActiveSensing(OnAppleMidiReceiveActiveSensing);
  AppleMIDI.OnReceiveReset(OnAppleMidiReceiveReset);
}

void setup()
{
#ifdef PEDALINO_DEBUG
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println("*** Pedalino(TM) ***");
#endif

  wifi_connect();

#ifdef PEDALINO_DEBUG
  Serial.flush();
#endif

  midi_connect();
}

void loop()
{
  // Listen to incoming messages from Arduino
  MIDI.read();

  // Listen to incoming messages from WiFi
  AppleMIDI.run();

  // Run HTTP Updater
  httpServer.handleClient();
}

