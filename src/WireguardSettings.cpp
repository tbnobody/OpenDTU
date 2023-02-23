#include "Configuration.h"
#include "MessageOutput.h"
#include <WireguardSettings.h>

WireguardSettingsClass::WireguardSettingsClass()
{
}

void WireguardSettingsClass::loop(){
    CONFIG_T& config = Configuration.get();
    if (config.Wg_Refresh){
        config.Wg_Refresh = false;
        delay(1000);
        yield();
        ESP.restart(); 
    }
}

void WireguardSettingsClass::init(){
    MessageOutput.print(F("Initialize Wireguard... "));
    if (Configuration.get().Wg_Enabled) {
        beginWireGuard();
    }
    MessageOutput.println(F("done"));
}

void WireguardSettingsClass::beginWireGuard(){
    //if (!wg.is_initialized()) {
        MessageOutput.print(F("Starting Wireguard...connect to: "));
        MessageOutput.print(Configuration.get().Wg_Endpoint_Address);
        MessageOutput.print(F(":"));
        MessageOutput.println(Configuration.get().Wg_Endpoint_Port);
        wg.begin(
            Configuration.get().Wg_Opendtu_Local_Ip, // IP address of the local interface
            Configuration.get().Wg_Opendtu_Private_Key, // Private key of the local interface
            Configuration.get().Wg_Endpoint_Address, // Address of the endpoint peer.
            Configuration.get().Wg_Endpoint_Public_Key, // Public key of the endpoint peer.
            Configuration.get().Wg_Endpoint_Port); // Port pf the endpoint peer.
    //}
}

void WireguardSettingsClass::endWireGuard(){
    //if (wg.is_initialized()){
        MessageOutput.print(F("Stopping Wireguard... "));
        //wg.end();
    //}
}

WireguardSettingsClass WireguardSettings;