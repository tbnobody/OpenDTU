// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "IntegrationsGoeController.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "NetworkSettings.h"
#include <Hoymiles.h>

IntegrationsGoeControllerClass IntegrationsGoeController;

IntegrationsGoeControllerClass::IntegrationsGoeControllerClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&IntegrationsGoeControllerClass::loop, this))
{
}

void IntegrationsGoeControllerClass::init(Scheduler& scheduler)
{
    using std::placeholders::_1;

    NetworkSettings.onEvent(std::bind(&IntegrationsGoeControllerClass::NetworkEvent, this, _1));

    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Integrations.GoeControllerUpdateInterval * TASK_SECOND);
    _loopTask.enable();
}

void IntegrationsGoeControllerClass::NetworkEvent(network_event event)
{
    switch (event) {
    case network_event::NETWORK_GOT_IP:
        _networkConnected = true;
        break;
    case network_event::NETWORK_DISCONNECTED:
        _networkConnected = false;
        break;
    default:
        break;
    }
}

void IntegrationsGoeControllerClass::loop()
{
    const auto& integrationsConfig = Configuration.get().Integrations;

    const bool reachable = Datastore.getIsAllEnabledReachable();

    _loopTask.setInterval((reachable ? integrationsConfig.GoeControllerUpdateInterval : std::min(integrationsConfig.GoeControllerUpdateInterval, 5U)) * TASK_SECOND);

    if (!integrationsConfig.GoeControllerEnabled) {
        return;
    }

    if (!_networkConnected || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    const auto value = reachable ? Datastore.getTotalAcPowerEnabled() : 0;

    //                                      home,    grid, car,  relais, solar
    // ecp is an array of numbers or null: [{power}, null, null, null, {power}]
    // setting the home category to the power should be configurable
    // url is this: http://{hostname}/api/set?ecp=

    auto url = "http://" + String(integrationsConfig.GoeControllerHostname) + "/api/set?ecp=";

    url += "[";
    url += integrationsConfig.GoeControllerPublishHomeCategory ? String(value) : "null";
    url += ",null,null,null,";
    url += value;
    url += "]";

    const auto timeout = std::max(2U, std::min(integrationsConfig.GoeControllerUpdateInterval-1, 3U)) * 1000U;

    _http.setConnectTimeout(timeout);
    _http.setTimeout(timeout);
    _http.setReuse(true);
    _http.begin(url);

    int httpCode = _http.GET();

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            MessageOutput.println("go-e Controller updated");
        } else {
            MessageOutput.printf("HTTP error: %d\n", httpCode);
        }
    } else {
        MessageOutput.println("HTTP error");
    }
}
