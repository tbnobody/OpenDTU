#include "Configuration.h"
#include "MessageOutput.h"
#include <WireguardSettings.h>

WireguardSettingsClass::WireguardSettingsClass()
{
}

void WireguardSettingsClass::loop(){
    if (Configuration.get().Wg_Enabled) {
        beginWireGuard();
    } else {
        wg.end();
    }
}

void WireguardSettingsClass::init(){ 
    // Configure & Start Wireguard Client
    if (Configuration.get().Wg_Enabled) {
        beginWireGuard();
    }
}

void WireguardSettingsClass::beginWireGuard(){
    if (!wg.is_initialized()) {
        wg.begin(
            Configuration.get().Wg_Opendtu_Local_Ip, // IP address of the local interface
            Configuration.get().Wg_Opendtu_Private_Key, // Private key of the local interface
            Configuration.get().Wg_Endpoint_Address, // Address of the endpoint peer.
            Configuration.get().Wg_Endpoint_Public_Key, // Public key of the endpoint peer.
            Configuration.get().Wg_Endpoint_Port); // Port pf the endpoint peer.
    }
}

WireguardSettingsClass WireguardSettings;