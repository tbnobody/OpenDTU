#include "LanSettings.h"
#include "Configuration.h"
#include "defaults.h"
#include <ETH.h>
#include <AsyncTCP.h>
#include <memory>

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

LanSettingsClass Lan;

LanSettingsClass::LanSettingsClass() {
	bEthConnected = false;
	bFirstLoop = true;
}

void LanSettingsClass::init() {
	// todo: only set static IP when DHCP server is unreachable 
	local_ip	= IPAddress (192, 168,   1,  17);
	gateway		= IPAddress (192, 168,   1,   1);
	subnet		= IPAddress (255, 255, 255,   0);
	dns1		  = IPAddress (192, 168,   1,   1);
	dns2		  = IPAddress (192, 168,   1,   1);

	WiFi.onEvent(std::bind(&LanSettingsClass::WiFiEvent, this, std::placeholders::_1));

}

void LanSettingsClass::firstLoop() {
	if(false == bFirstLoop) {
		return;
	}
	else {
		// we have to do ETH.begin() in the "FirstLoop",
		// because MqttSettingsClass::WiFiEvent callback has to register before.
		// There is the MQTT "connection detection"
		ETH.begin();
	}
	bFirstLoop = false;
}

void LanSettingsClass::loop() {
	firstLoop();
}

bool LanSettingsClass::isConnected() {
	return bEthConnected;
}

// look @ https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiClientEvents/WiFiClientEvents.ino 
void LanSettingsClass::WiFiEvent(WiFiEvent_t event) {

	switch (event) {

		case ARDUINO_EVENT_ETH_START: // 18
			if(true == bDebugMode) {
				Serial.println(F("ETH: Started"));
			}
			//set eth hostname here
			ETH.setHostname("openDTU-eth");
//          ETH.config(local_ip, gateway, subnet, dns1, dns2);
		break;

		case ARDUINO_EVENT_ETH_STOP: // 19
			if(true == bDebugMode) {
				Serial.println(F("ETH: Stopped"));
			}
			bEthConnected = false;
			break;

		case ARDUINO_EVENT_ETH_CONNECTED: // 20
			if(true == bDebugMode) {
				Serial.println(F("ETH: Connected"));
			}
			break;

		case ARDUINO_EVENT_ETH_DISCONNECTED: // 21
			if(true == bDebugMode) {
				Serial.println(F("ETH: Disconnected"));
			}
			bEthConnected = false;
			break;

		case ARDUINO_EVENT_ETH_GOT_IP: // 22
			if(true == bDebugMode) {
				Serial.print(F("ETH: MAC: "));
				Serial.print(ETH.macAddress());
				Serial.print(F(", IPv4: "));
				Serial.print(ETH.localIP());
				if (ETH.fullDuplex()) {
					Serial.print(F(", FULL_DUPLEX"));
				}
				Serial.print(F(", "));
				Serial.print(ETH.linkSpeed());
				Serial.println(F("Mbps"));
				bEthConnected = true;
			}
			break;

		default:
			break;
	}
}
