// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "ZeroExportGridSource.h"
#include <WiFiUdp.h>
#include <optional>

// Grid power source receiving Shelly Local Network Messaging (LNM) status
// messages over UDP multicast.
//
// Wire protocol and JSON payload are documented at:
// - https://shelly-api-docs.shelly.cloud/gen2/General/LocalNetworkMessaging/
// - https://shelly-api-docs.shelly.cloud/gen2/DynamicComponents/LNM/
//
// The multicast group and port come from the Zero-Export configuration
// (ZeroExport.ShellyLnm.GroupAddress / GroupPort).
class ZeroExportShellyLnmClass : public ZeroExportGridSource {
public:
    ZeroExportShellyLnmClass();
    void start() override;
    void stop() override;
    bool isRunning() const override;
    // Drain queued datagrams without blocking the scheduler.
    void loop() override;

private:
    // Validate and decode one Shelly LNM datagram.
    void processPacket(const uint8_t* data, size_t len);

    WiFiUDP _udp; // Multicast socket.
    bool _listening = false; // Whether the multicast group is joined.
    char _listenAddr[16] = { 0 }; // Endpoint currently joined, for diagnostics.
    uint16_t _listenPort = 0; // Endpoint currently joined, for diagnostics.;
    String _type; // Type of power measurement (e.g., "act_power", "total_act_power", "a_act_power", "b_act_power", "c_act_power").
};

extern ZeroExportShellyLnmClass ZeroExportShellyLnm;