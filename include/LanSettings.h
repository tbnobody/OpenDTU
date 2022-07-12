#pragma once

#include <WiFi.h>
#include <AsyncTCP.h>

class LanSettingsClass {
public:
    LanSettingsClass();
    void init();
    void loop();
	  void firstLoop();
    void applyConfig();
	  bool isConnected();

private:
	  void WiFiEvent(WiFiEvent_t event);

	  bool bEthConnected;
	  bool bFirstLoop;
	  bool bDebugMode = true;

	  IPAddress local_ip;
	  IPAddress gateway;
	  IPAddress subnet;
	  IPAddress dns1;
	  IPAddress dns2;

};

extern LanSettingsClass Lan;
