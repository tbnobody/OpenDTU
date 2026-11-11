// SPDX-License-Identifier: GPL-2.0-or-later
#include "CmtLinkRecovery.h"
#include "Hoymiles.h"
#include "HoymilesRadio_CMT.h"
#include "commands/ChannelChangeCommand.h"
#include "commands/RfInfoCommand.h"
#include "inverters/InverterAbstract.h"
#include <esp_log.h>

#undef TAG
static const char* TAG = "hoymiles";

void CmtLinkRecovery::onUnreachable(InverterAbstract* inv)
{
    HoymilesRadio_CMT* radio = Hoymiles.getRadioCmt();
    if (inv == nullptr || radio == nullptr || !radio->isInitialized()) {
        return;
    }

    if (!(inv->getEnableCommands() || inv->getEnablePolling())) {
        return;
    }

    CmtLinkState& link = inv->CmtLink;

    if (isHeard(inv)) {
        link.failCount = 0;
        ESP_LOGI(TAG, "%s: not answering but heard %u ms ago, no search",
            inv->serialString().c_str(), static_cast<unsigned>(millis() - link.lastHeardMs));
        return;
    }

    if (link.frequency != 0 && ++link.failCount >= FAILS_BEFORE_RESCAN) {
        ESP_LOGW(TAG, "%s: no answer on %.3f MHz any more, searching again",
            inv->serialString().c_str(), link.frequency / 1000000.0);
        link.frequency = 0;
        link.failCount = 0;
        link.probeStep = 0;
    }

    for (uint8_t i = 0; i < PROBES_PER_CYCLE; i++) {
        uint32_t frequency = 0;
        if (!radio->getProbeFrequency(link.probeStep, frequency, link.startChannel)) {
            link.probeStep = 0;
            if (!radio->getProbeFrequency(link.probeStep, frequency, link.startChannel)) {
                return;
            }
        }
        link.probeStep++;

        auto cmd = inv->getRadio()->prepareCommand<RfInfoCommand>(inv);
        cmd->setProbeFrequency(frequency);
        inv->getRadio()->enqueCommand(cmd);
    }
}

bool CmtLinkRecovery::isHeard(const InverterAbstract* inv)
{
    return inv != nullptr && inv->wasHeardRecently(HEARD_POLL_CYCLES);
}

void CmtLinkRecovery::onProbeHit(InverterAbstract* inv, const uint32_t frequency)
{
    HoymilesRadio_CMT* radio = Hoymiles.getRadioCmt();
    if (inv == nullptr || radio == nullptr) {
        return;
    }

    CmtLinkState& link = inv->CmtLink;

    const uint8_t hitChannel = radio->getChannelFromFrequency(frequency);
    const uint8_t targetChannel = radio->getChannelFromFrequency(radio->getInverterTargetFrequency());
    const bool onTargetRing = hitChannel != 0xFF && targetChannel != 0xFF
        && hitChannel + 1 >= targetChannel && hitChannel <= targetChannel + 1;

    const uint32_t learned = onTargetRing ? 0 : frequency;

    if (link.frequency != learned) {
        ESP_LOGI(TAG, "%s answered on %.3f MHz (DTU frequency %.3f MHz)",
            inv->serialString().c_str(), frequency / 1000000.0, radio->getInverterTargetFrequency() / 1000000.0);
    }

    link.frequency = learned;
    link.failCount = 0;
    link.probeStep = 0;

    if (learned == 0) {
        link.acceptedNoLink = 0;
        link.moveAttempts = 0;
    } else if (link.moveAttempts < MAX_MOVE_ATTEMPTS) {
        link.moveAttempts++;
        enqueueChannelChange(inv, learned);
    }
}

void CmtLinkRecovery::enqueueChannelChange(InverterAbstract* inv, const uint32_t frequency)
{
    HoymilesRadio_CMT* radio = Hoymiles.getRadioCmt();
    if (inv == nullptr || radio == nullptr || !radio->isInitialized()) {
        return;
    }

    const CmtLinkState& link = inv->CmtLink;

    const uint8_t channel = radio->getChannelFromFrequency(radio->getInverterTargetFrequency());
    if (channel == 0xFF) {
        return;
    }
    const int16_t moduleChannel = static_cast<int16_t>(channel) + link.channelOffset;
    if (moduleChannel < 1 || moduleChannel > 0xFE) {
        return;
    }

    auto cmd = inv->getRadio()->prepareCommand<ChannelChangeCommand>(inv);
    cmd->setCountryMode(radio->getCountryMode());
    cmd->setBandBytes(link.profile, link.startChannel);
    cmd->setChannel(static_cast<uint8_t>(moduleChannel));
    cmd->setFrequency(frequency);
    inv->getRadio()->enqueCommand(cmd);
}

void CmtLinkRecovery::onChannelChangeAccepted(InverterAbstract* inv, const uint8_t profile, const uint8_t startChannel)
{
    HoymilesRadio_CMT* radio = Hoymiles.getRadioCmt();
    if (inv == nullptr || radio == nullptr) {
        return;
    }

    CmtLinkState& link = inv->CmtLink;
    link.profile = profile;
    link.startChannel = startChannel;

    if (++link.acceptedNoLink >= ACCEPTED_BEFORE_OFFSET_SWITCH) {
        link.acceptedNoLink = 0;
        link.channelOffset = (link.channelOffset == 0) ? GRID_OFFSET_ALTERNATIVE : 0;
        ESP_LOGW(TAG, "%s: channel change accepted %u times but inverter not reachable, grid offset now %d channels",
            inv->serialString().c_str(), static_cast<unsigned>(ACCEPTED_BEFORE_OFFSET_SWITCH), static_cast<int>(link.channelOffset));
    }

    ESP_LOGI(TAG, "%s: channel change accepted (module profile 0x%02x, start channel %u)",
        inv->serialString().c_str(), profile, static_cast<unsigned>(startChannel));

    auto cmd = inv->getRadio()->prepareCommand<RfInfoCommand>(inv);
    cmd->setProbeFrequency(radio->getInverterTargetFrequency());
    inv->getRadio()->enqueCommand(cmd);
}

void CmtLinkRecovery::onChannelChangeRejected(InverterAbstract* inv, const uint8_t profile, const uint8_t startChannel)
{
    if (inv == nullptr) {
        return;
    }

    CmtLinkState& link = inv->CmtLink;
    const bool changed = (link.profile != profile) || (link.startChannel != startChannel);
    link.profile = profile;
    link.startChannel = startChannel;

    ESP_LOGW(TAG, "%s: channel change rejected, module has profile 0x%02x and start channel %u",
        inv->serialString().c_str(), profile, static_cast<unsigned>(startChannel));

    if (changed) {
        enqueueChannelChange(inv, 0);
    }
}
