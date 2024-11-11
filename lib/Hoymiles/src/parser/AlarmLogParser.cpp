// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

/*
This parser is used to parse the response of 'AlarmDataCommand'.

Data structure:
* wcode:
  * right 8 bit: Event ID
  * bit 13: Start time = PM (12h has to be added to start time)
  * bit 12: End time = PM (12h has to be added to start time)
* Start: 12h based start time of the event (PM indicator in wcode)
* End: 12h based start time of the event (PM indicator in wcode)

00   01 02 03 04   05 06 07 08   09   10 11   12 13   14 15   16 17   18 19   20   21   22   23   24 25   26   27 28 29 30 31
                                              00 01   02 03   04 05   06 07   08   09   10   11
                                              |<-------------- First log entry -------------->|   |<->|
-----------------------------------------------------------------------------------------------------------------------------
95   80 14 82 66   80 14 33 28   01   00 01   80 01   00 01   91 EA   91 EA   00   00   00   00   00 8F   65   -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^^^^   ^^^^^           ^^^^^   ^^^^^   ^^   ^^   ^^   ^^   ^^^^^   ^^
ID   Source Addr   Target Addr   Idx  ?       wcode   ?       Start   End     ?    ?    ?    ?    wcode   CRC8
*/
#include "AlarmLogParser.h"
#include "../Hoymiles.h"
#include <cstring>

const std::array<const AlarmMessage_t, ALARM_MSG_COUNT> AlarmLogParser::_alarmMessages = { {
    { AlarmMessageType_t::ALL, 1, "Inverter start", "Wechselrichter gestartet", "L'onduleur a démarré" },
    { AlarmMessageType_t::ALL, 2, "Time calibration", "Zeitabgleich", "" },
    { AlarmMessageType_t::ALL, 3, "EEPROM reading and writing error during operation", "", "" },
    { AlarmMessageType_t::ALL, 4, "Offline", "Offline", "Non connecté" },

    { AlarmMessageType_t::ALL, 11, "Grid voltage surge", "Netz: Überspannungsimpuls", "" },
    { AlarmMessageType_t::ALL, 12, "Grid voltage sharp drop", "Netz: Spannungseinbruch", "" },
    { AlarmMessageType_t::ALL, 13, "Grid frequency mutation", "Netz: Frequenzänderung", "" },
    { AlarmMessageType_t::ALL, 14, "Grid phase mutation", "Netz: Phasenänderung", "" },
    { AlarmMessageType_t::ALL, 15, "Grid transient fluctuation", "Netz: vorübergehende Schwankung", "" },

    { AlarmMessageType_t::ALL, 36, "INV overvoltage or overcurrent", "", "" },

    { AlarmMessageType_t::ALL, 46, "FB overvoltage", "FB Überspannung", "" },
    { AlarmMessageType_t::ALL, 47, "FB overcurrent", "FB Überstrom", "" },
    { AlarmMessageType_t::ALL, 48, "FB clamp overvoltage", "", "" },
    { AlarmMessageType_t::ALL, 49, "FB clamp overvoltage", "", "" },

    { AlarmMessageType_t::ALL, 61, "Calibration parameter error", "", "" },
    { AlarmMessageType_t::ALL, 62, "System configuration parameter error", "", "" },
    { AlarmMessageType_t::ALL, 63, "Abnormal power generation data", "", "" },

    { AlarmMessageType_t::ALL, 71, "Grid overvoltage load reduction (VW) function enable", "", "" },
    { AlarmMessageType_t::ALL, 72, "Power grid over-frequency load reduction (FW) function enable", "", "" },
    { AlarmMessageType_t::ALL, 73, "Over-temperature load reduction (TW) function enable", "", "" },

    { AlarmMessageType_t::ALL, 95, "PV-1: Module in suspected shadow", "", "" },
    { AlarmMessageType_t::ALL, 96, "PV-2: Module in suspected shadow", "", "" },
    { AlarmMessageType_t::ALL, 97, "PV-3: Module in suspected shadow", "", "" },
    { AlarmMessageType_t::ALL, 98, "PV-4: Module in suspected shadow", "", "" },

    { AlarmMessageType_t::ALL, 121, "Over temperature protection", "Übertemperaturschutz", "Protection antisurchauffe" },
    { AlarmMessageType_t::ALL, 122, "Microinverter is suspected of being stolen", "", "" },
    { AlarmMessageType_t::ALL, 123, "Locked by remote control", "", "" },
    { AlarmMessageType_t::ALL, 124, "Shut down by remote control", "Durch Fernsteuerung abgeschaltet", "Arrêt par télécommande" },
    { AlarmMessageType_t::ALL, 125, "Grid configuration parameter error", "Parameterfehler bei der Konfiguration des Elektrizitätsnetzes", "Erreur de paramètre de configuration du réseau" },
    { AlarmMessageType_t::ALL, 126, "Software error code 126", "", "" },
    { AlarmMessageType_t::ALL, 127, "Firmware error", "Firmwarefehler", "Erreur du micrologiciel" },
    { AlarmMessageType_t::ALL, 128, "Hardware configuration error", "", "" },
    { AlarmMessageType_t::ALL, 129, "Abnormal bias", "Abnormaler Trend", "Polarisation anormale" },
    { AlarmMessageType_t::ALL, 130, "Offline", "Offline", "Non connecté" },

    { AlarmMessageType_t::ALL, 141, "Grid: Grid overvoltage", "Netz: Netzüberspannung", "Réseau: Surtension du réseau" },
    { AlarmMessageType_t::ALL, 142, "Grid: 10 min value grid overvoltage", "Netz: 10 Minuten-Mittelwert der Netzüberspannung", "Réseau: Valeur de surtension du réseau pendant 10 min" },
    { AlarmMessageType_t::ALL, 143, "Grid: Grid undervoltage", "Netz: Netzunterspannung", "Réseau: Sous-tension du réseau" },
    { AlarmMessageType_t::ALL, 144, "Grid: Grid overfrequency", "Netz: Netzüberfrequenz", "Réseau: Surfréquence du réseau" },
    { AlarmMessageType_t::ALL, 145, "Grid: Grid underfrequency", "Netz: Netzunterfrequenz", "Réseau: Sous-fréquence du réseau" },
    { AlarmMessageType_t::ALL, 146, "Grid: Rapid grid frequency change rate", "Netz: Schnelle Wechselrate der Netzfrequenz", "Réseau: Taux de fluctuation rapide de la fréquence du réseau" },
    { AlarmMessageType_t::ALL, 147, "Grid: Power grid outage", "Netz: Elektrizitätsnetzausfall", "Réseau: Panne du réseau électrique" },
    { AlarmMessageType_t::ALL, 148, "Grid: Grid disconnection", "Netz: Netztrennung", "Réseau: Déconnexion du réseau" },
    { AlarmMessageType_t::ALL, 149, "Grid: Island detected", "Netz: Inselbetrieb festgestellt", "Réseau: Détection d’îlots" },

    { AlarmMessageType_t::ALL, 150, "DCI exceeded", "", "" },
    { AlarmMessageType_t::ALL, 152, "Grid: Phase angle difference between two phases exceeded 5° >10 times", "", "" },
    { AlarmMessageType_t::HMT, 171, "Grid: Abnormal phase difference between phase to phase", "", "" },
    { AlarmMessageType_t::ALL, 181, "Abnormal insulation impedance", "", "" },
    { AlarmMessageType_t::ALL, 182, "Abnormal grounding", "", "" },

    { AlarmMessageType_t::ALL, 205, "MPPT-A: Input overvoltage", "MPPT-A: Eingangsüberspannung", "MPPT-A: Surtension d’entrée" },
    { AlarmMessageType_t::ALL, 206, "MPPT-B: Input overvoltage", "MPPT-B: Eingangsüberspannung", "MPPT-B: Surtension d’entrée" },
    { AlarmMessageType_t::ALL, 207, "MPPT-A: Input undervoltage", "MPPT-A: Eingangsunterspannung", "MPPT-A: Sous-tension d’entrée" },
    { AlarmMessageType_t::ALL, 208, "MPPT-B: Input undervoltage", "MPPT-B: Eingangsunterspannung", "MPPT-B: Sous-tension d’entrée" },

    { AlarmMessageType_t::ALL, 209, "PV-1: No input", "PV-1: Kein Eingang", "PV-1: Aucune entrée" },
    { AlarmMessageType_t::ALL, 210, "PV-2: No input", "PV-2: Kein Eingang", "PV-2: Aucune entrée" },
    { AlarmMessageType_t::ALL, 211, "PV-3: No input", "PV-3: Kein Eingang", "PV-3: Aucune entrée" },
    { AlarmMessageType_t::ALL, 212, "PV-4: No input", "PV-4: Kein Eingang", "PV-4: Aucune entrée" },

    { AlarmMessageType_t::ALL, 213, "MPPT-A: PV-1 & PV-2 abnormal wiring", "MPPT-A: Verdrahtungsfehler bei PV-1 und PV-2", "MPPT-A: Câblages photovoltaïques 1 et 2 anormaux" },
    { AlarmMessageType_t::ALL, 214, "MPPT-B: PV-3 & PV-4 abnormal wiring", "MPPT-B: Verdrahtungsfehler bei PV-3 und PV-4", "MPPT-B: Câblages photovoltaïques 3 et 4 anormaux" },

    { AlarmMessageType_t::ALL, 215, "PV-1: Input overvoltage", "PV-1: Eingangsüberspannung", "PV-1: Surtension d’entrée" },
    { AlarmMessageType_t::HMT, 215, "MPPT-C: Input overvoltage", "MPPT-C: Eingangsüberspannung", "MPPT-C: Surtension d’entrée" },
    { AlarmMessageType_t::ALL, 216, "PV-1: Input undervoltage", "PV-1: Eingangsunterspannung", "PV-1: Sous-tension d’entrée" },
    { AlarmMessageType_t::HMT, 216, "MPPT-C: Input undervoltage", "MPPT-C: Eingangsunterspannung", "MPPT-C: Sous-tension d’entrée" },
    { AlarmMessageType_t::ALL, 217, "PV-2: Input overvoltage", "PV-2: Eingangsüberspannung", "PV-2: Surtension d’entrée" },
    { AlarmMessageType_t::HMT, 217, "PV-5: No input", "PV-5: Kein  Eingang", "PV-5: Aucune entrée" },
    { AlarmMessageType_t::ALL, 218, "PV-2: Input undervoltage", "PV-2: Eingangsunterspannung", "PV-2: Sous-tension d’entrée" },
    { AlarmMessageType_t::HMT, 218, "PV-6: No input", "PV-6: Kein Eingang", "PV-6: Aucune entrée" },
    { AlarmMessageType_t::ALL, 219, "PV-3: Input overvoltage", "PV-3: Eingangsüberspannung", "PV-3: Surtension d’entrée" },
    { AlarmMessageType_t::HMT, 219, "MPPT-C: PV-5 & PV-6 abnormal wiring", "", "" },
    { AlarmMessageType_t::ALL, 220, "PV-3: Input undervoltage", "PV-3: Eingangsunterspannung", "PV-3: Sous-tension d’entrée" },
    { AlarmMessageType_t::ALL, 221, "PV-4: Input overvoltage", "PV-4: Eingangsüberspannung", "PV-4: Surtension d’entrée" },
    { AlarmMessageType_t::HMT, 221, "Abnormal wiring of grid neutral line", "", "" },
    { AlarmMessageType_t::ALL, 222, "PV-4: Input undervoltage", "PV-4: Eingangsunterspannung", "PV-4: Sous-tension d’entrée" },

    { AlarmMessageType_t::ALL, 301, "FB-A: internal short circuit failure", "", "" },
    { AlarmMessageType_t::ALL, 302, "FB-B: internal short circuit failure", "", "" },

    { AlarmMessageType_t::ALL, 303, "FB-A: overcurrent protection failure", "", "" },
    { AlarmMessageType_t::ALL, 304, "FB-B: overcurrent protection failure", "", "" },

    { AlarmMessageType_t::ALL, 305, "FB-A: clamp circuit failure", "", "" },
    { AlarmMessageType_t::ALL, 306, "FB-B: clamp circuit failure", "", "" },

    { AlarmMessageType_t::ALL, 307, "INV power device failure", "", "" },
    { AlarmMessageType_t::ALL, 308, "INV overcurrent or overvoltage protection failure", "", "" },

    { AlarmMessageType_t::ALL, 309, "Hardware error code 309", "Hardwarefehlercode 309", "" },
    { AlarmMessageType_t::ALL, 310, "Hardware error code 310", "Hardwarefehlercode 310", "" },
    { AlarmMessageType_t::ALL, 311, "Hardware error code 311", "Hardwarefehlercode 311", "" },
    { AlarmMessageType_t::ALL, 312, "Hardware error code 312", "Hardwarefehlercode 312", "" },
    { AlarmMessageType_t::ALL, 313, "Hardware error code 313", "Hardwarefehlercode 313", "" },
    { AlarmMessageType_t::ALL, 314, "Hardware error code 314", "Hardwarefehlercode 314", "" },

    { AlarmMessageType_t::ALL, 1111, "Repeater", "", "" },

    { AlarmMessageType_t::ALL, 2000, "Standby", "", "" },
    { AlarmMessageType_t::ALL, 2001, "Standby", "", "" },
    { AlarmMessageType_t::ALL, 2002, "Standby", "", "" },
    { AlarmMessageType_t::ALL, 2003, "Standby", "", "" },
    { AlarmMessageType_t::ALL, 2004, "Standby", "", "" },

    { AlarmMessageType_t::ALL, 3001, "Reset", "", "" },
    { AlarmMessageType_t::ALL, 3002, "Reset", "", "" },
    { AlarmMessageType_t::ALL, 3003, "Reset", "", "" },
    { AlarmMessageType_t::ALL, 3004, "Reset", "", "" },

    { AlarmMessageType_t::ALL, 5011, "PV-1: MOSFET overcurrent (II)", "PV-1: MOSFET Überstrom (II)", "" },
    { AlarmMessageType_t::ALL, 5012, "PV-2: MOSFET overcurrent (II)", "PV-2: MOSFET Überstrom (II)", "" },
    { AlarmMessageType_t::ALL, 5013, "PV-3: MOSFET overcurrent (II)", "PV-3: MOSFET Überstrom (II)", "" },
    { AlarmMessageType_t::ALL, 5014, "PV-4: MOSFET overcurrent (II)", "PV-4: MOSFET Überstrom (II)", "" },
    { AlarmMessageType_t::ALL, 5020, "H-bridge MOSFET overcurrent or H-bridge overvoltage", "H-Brücken-MOSFET-Überstrom oder H-Brücken-Überspannung", "" },

    { AlarmMessageType_t::ALL, 5041, "PV-1: current overcurrent (II)", "", "" },
    { AlarmMessageType_t::ALL, 5042, "PV-2: current overcurrent (II)", "", "" },
    { AlarmMessageType_t::ALL, 5043, "PV-3: current overcurrent (II)", "", "" },
    { AlarmMessageType_t::ALL, 5044, "PV-4: current overcurrent (II)", "", "" },

    { AlarmMessageType_t::ALL, 5051, "PV-1: Overvoltage/Undervoltage", "", "" },
    { AlarmMessageType_t::ALL, 5052, "PV-2: Overvoltage/Undervoltage", "", "" },
    { AlarmMessageType_t::ALL, 5053, "PV-3: Overvoltage/Undervoltage", "", "" },
    { AlarmMessageType_t::ALL, 5054, "PV-4: Overvoltage/Undervoltage", "", "" },

    { AlarmMessageType_t::ALL, 5060, "Abnormal bias", "Abnormaler Trend", "Polarisation anormale" },
    { AlarmMessageType_t::ALL, 5070, "Over temperature protection", "Übertemperaturschutz", "Protection antisurchauffe" },
    { AlarmMessageType_t::ALL, 5080, "Grid Overvoltage/Undervoltage", "", "" },
    { AlarmMessageType_t::ALL, 5090, "Grid Overfrequency/Underfrequency", "", "" },
    { AlarmMessageType_t::ALL, 5100, "Island detected", "Inselbetrieb festgestellt", "Détection d’îlots" },
    { AlarmMessageType_t::ALL, 5110, "GFDI failure", "", "" },
    { AlarmMessageType_t::ALL, 5120, "EEPROM reading and writing error", "", "" },

    { AlarmMessageType_t::ALL, 5141, "FB clamp overvoltage", "", "" },
    { AlarmMessageType_t::ALL, 5142, "FB clamp overvoltage", "", "" },
    { AlarmMessageType_t::ALL, 5143, "FB clamp overvoltage", "", "" },
    { AlarmMessageType_t::ALL, 5144, "FB clamp overvoltage", "", "" },

    { AlarmMessageType_t::ALL, 5150, "10 min value grid overvoltage", "10 Minuten-Mittelwert der Netzüberspannung", "Valeur de surtension du réseau pendant 10 min" },
    { AlarmMessageType_t::ALL, 5160, "Grid transient fluctuation", "", "" },

    { AlarmMessageType_t::ALL, 5200, "Firmware error", "Firmwarefehler", "Erreur du micrologiciel" },

    { AlarmMessageType_t::ALL, 5511, "PV-1: MOSFET overcurrent-H", "PV-1: MOSFET Überstrom-H", "" },
    { AlarmMessageType_t::ALL, 5512, "PV-2: MOSFET overcurrent-H", "PV-2: MOSFET Überstrom-H", "" },
    { AlarmMessageType_t::ALL, 5513, "PV-3: MOSFET overcurrent-H", "PV-3: MOSFET Überstrom-H", "" },
    { AlarmMessageType_t::ALL, 5514, "PV-4: MOSFET overcurrent-H", "PV-4: MOSFET Überstrom-H", "" },
    { AlarmMessageType_t::ALL, 5520, "H-bridge MOSFET overcurrent or H-bridge overvoltage", "H-Brücken-MOSFET-Überstrom oder H-Brücken-Überspannung", "" },

    { AlarmMessageType_t::ALL, 8310, "Shut down by remote control", "Durch Fernsteuerung abgeschaltet", "Arrêt par télécommande" },
    { AlarmMessageType_t::ALL, 8320, "Locked by remote control", "", "" },
    { AlarmMessageType_t::ALL, 9000, "Microinverter is suspected of being stolen", "", "" },
} };

AlarmLogParser::AlarmLogParser()
    : Parser()
{
    clearBuffer();
}

void AlarmLogParser::clearBuffer()
{
    memset(_payloadAlarmLog, 0, ALARM_LOG_PAYLOAD_SIZE);
    _alarmLogLength = 0;
}

void AlarmLogParser::appendFragment(const uint8_t offset, const uint8_t* payload, const uint8_t len)
{
    if (offset + len > ALARM_LOG_PAYLOAD_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) stats packet too large for buffer (%d > %d)\r\n", __FILE__, __LINE__, offset + len, ALARM_LOG_PAYLOAD_SIZE);
        return;
    }
    memcpy(&_payloadAlarmLog[offset], payload, len);
    _alarmLogLength += len;
}

uint8_t AlarmLogParser::getEntryCount() const
{
    if (_alarmLogLength < 2) {
        return 0;
    }
    return (_alarmLogLength - 2) / ALARM_LOG_ENTRY_SIZE;
}

void AlarmLogParser::setLastAlarmRequestSuccess(const LastCommandSuccess status)
{
    _lastAlarmRequestSuccess = status;
}

LastCommandSuccess AlarmLogParser::getLastAlarmRequestSuccess() const
{
    return _lastAlarmRequestSuccess;
}

void AlarmLogParser::setMessageType(const AlarmMessageType_t type)
{
    _messageType = type;
}

void AlarmLogParser::getLogEntry(const uint8_t entryId, AlarmLogEntry_t& entry, const AlarmMessageLocale_t locale)
{
    const uint8_t entryStartOffset = 2 + entryId * ALARM_LOG_ENTRY_SIZE;

    const int timezoneOffset = getTimezoneOffset();

    HOY_SEMAPHORE_TAKE();

    const uint32_t wcode = static_cast<uint16_t>(_payloadAlarmLog[entryStartOffset]) << 8 | _payloadAlarmLog[entryStartOffset + 1];
    uint32_t startTimeOffset = 0;
    if (((wcode >> 13) & 0x01) == 1) {
        startTimeOffset = 12 * 60 * 60;
    }

    uint32_t endTimeOffset = 0;
    if (((wcode >> 12) & 0x01) == 1) {
        endTimeOffset = 12 * 60 * 60;
    }

    entry.MessageId = _payloadAlarmLog[entryStartOffset + 1];
    entry.StartTime = ((static_cast<uint16_t>(_payloadAlarmLog[entryStartOffset + 4]) << 8) | static_cast<uint16_t>(_payloadAlarmLog[entryStartOffset + 5])) + startTimeOffset + timezoneOffset;
    entry.EndTime = (static_cast<uint16_t>(_payloadAlarmLog[entryStartOffset + 6]) << 8) | static_cast<uint16_t>(_payloadAlarmLog[entryStartOffset + 7]);

    HOY_SEMAPHORE_GIVE();

    if (entry.EndTime > 0) {
        entry.EndTime += (endTimeOffset + timezoneOffset);
    }

    switch (locale) {
    case AlarmMessageLocale_t::DE:
        entry.Message = "Unbekannt";
        break;
    case AlarmMessageLocale_t::FR:
        entry.Message = "Inconnu";
        break;
    default:
        entry.Message = "Unknown";
    }

    for (auto& msg : _alarmMessages) {
        if (msg.MessageId == entry.MessageId) {
            if (msg.InverterType == _messageType) {
                entry.Message = getLocaleMessage(&msg, locale);
                break;
            } else if (msg.InverterType == AlarmMessageType_t::ALL) {
                entry.Message = getLocaleMessage(&msg, locale);
            }
        }
    }
}

String AlarmLogParser::getLocaleMessage(const AlarmMessage_t* msg, const AlarmMessageLocale_t locale) const
{
    if (locale == AlarmMessageLocale_t::DE) {
        return msg->Message_de[0] != '\0' ? msg->Message_de : msg->Message_en;
    }

    if (locale == AlarmMessageLocale_t::FR) {
        return msg->Message_fr[0] != '\0' ? msg->Message_fr : msg->Message_en;
    }

    return msg->Message_en;
}

int AlarmLogParser::getTimezoneOffset()
{
    // see: https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c/44063597#44063597

    time_t gmt, rawtime = time(NULL);
    struct tm* ptm;

    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);

    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);

    return static_cast<int>(difftime(rawtime, gmt));
}
