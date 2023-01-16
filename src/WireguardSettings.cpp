#include <WireguardSettings.h>
#include "Configuration.h"
#include "MessageOutput.h"

WireguardSettingsClass::WireguardSettingsClass()
{
}

void WireguardSettingsClass::loop(){
}

void WireguardSettingsClass::init()
{ // Configure & Start Wireguard Client
    if (!wg.is_initialized() && Configuration.get().Wg_Enabled) {
        beginWireGuard();
    }
}

void WireguardSettingsClass::beginWireGuard(){
    wg.begin(
        Configuration.get().Wg_Local_Ip,           // IP address of the local interface
        Configuration.get().Wg_Opendtu_Private_Key,// Private key of the local interface
        Configuration.get().Wg_Endpoint_Address,   // Address of the endpoint peer.
        Configuration.get().Wg_Endpoint_Public_Key,// Public key of the endpoint peer.
        Configuration.get().Wg_Endpoint_Port);     // Port pf the endpoint peer.
}

WireguardSettingsClass WireguardSettings;