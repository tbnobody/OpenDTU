#ifndef __POWERCONTROLPLUGIN_H__
#define __POWERCONTROLPLUGIN_H__

#include "plugin.h"

typedef struct {
    uint64_t inverterSerial = 0;
    String inverterSerialString;
    String meterSerial;
    float limit = 0.0;
    uint32_t threshold = 0;
    float consumption = 0.0;
    float production = 0.0;
    bool update = false;
} powercontrolstruct;

class PowercontrolPlugin2 : public Plugin {
    enum pluginIds { INVERTER_,
        INVERTERSTRING_,
        POWERLIMIT_ };

public:
    PowercontrolPlugin2()
        : Plugin(33, "powercontrol2")
    {
    }

    void setup() { }
    void loop()
    {
        if (powercontrol.update) {
            enqueueMessage((char*)"updateLimit",(char*)"true",true);
            powercontrol.update = false;
            calcLimit();
        }
    }
    void calcLimit()
    {
        MessageOutput.printf("powercontrol calcLimit: consumption=%f production=%f limit=%f\n",powercontrol.consumption,powercontrol.production,powercontrol.limit);
        // TODO: do some magic calculation here
        // :/
        // float newlimit = magicFunction(powercontrol.production,powercontrol.consumption);

        float newLimit = powercontrol.consumption;
        float threshold = std::abs(powercontrol.limit - newLimit);
        if(threshold <= powercontrol.threshold) {
            MessageOutput.printf("powercontrol newlimit(%f) within threshold(%f) -> no limit change\n",newLimit,threshold);
        } else {
            MessageOutput.printf("powercontrol setting limit to %f\n",newLimit);
            powercontrol.limit=newLimit;
            publishLimit(powercontrol);
        }
    }
    void publishLimit(powercontrolstruct pc)
    {
        PluginMessage m(*this);
        m.add(StringValue(INVERTERSTRING,pc.inverterSerialString));
        m.add(FloatValue(POWERLIMIT, pc.limit));
        publishMessage(m);
    }

    void internalCallback(std::shared_ptr<PluginMessage> message)
    {
        MessageOutput.printf("powercontrol internalCallback: %d\n", message->getSenderId());
    }           

    void internalDataCallback(PluginMessage* message)
    {
        // DBGPRINTMESSAGELN(DBG_INFO,"powercontroller",message);
        if (message->has(PluginIds::PluginMeter, PluginMeterIds::METER_POWER)) {
            String meterserial = message->getDataAs<StringValue>(PluginMeterIds::METER_SERIAL).value;
            if (powercontrol.meterSerial.equals(meterserial)) {
                powercontrol.consumption = message->getDataAs<FloatValue>(PluginMeterIds::METER_POWER).value;
                powercontrol.update = true;
                MessageOutput.printf("powercontrol got consumption: %f\n", powercontrol.consumption);
            }
        } else if (message->has(PluginIds::PluginInverter, PluginInverterIds::ACPOWER_PRODUCTION)) {
            String inverterSerial = message->getDataAs<StringValue>(PluginInverterIds::ACPOWER_INVERTERSTRING).value;
            if (inverterSerial.equals(powercontrol.inverterSerialString)) {
                {
                    powercontrol.production = message->getDataAs<FloatValue>(PluginInverterIds::ACPOWER_PRODUCTION).value;
                    powercontrol.update = true;
                    MessageOutput.printf("powercontrol got production: %f\n", powercontrol.production);
                }
            }
        } else {
            MessageOutput.printf("powercontrol unhandled message from sender=%d\n",message->getSenderId());
        }
    }
    void initPowercontrol()
    {
        powercontrol.inverterSerialString = inverter_serial;
        powercontrol.meterSerial = meter_serial;
        powercontrol.update = false;
        powercontrol.limit = 0;
        powercontrol.threshold = threshold;
    }
    void saveSettings(JsonObject settings)
    {
        settings[F("meter_serial")] = meter_serial;
        settings[F("inverter_serial")] = inverter_serial;
        settings[F("threshold")] = threshold;
    }
    void loadSettings(JsonObject settings)
    {
        if (settings.containsKey(F("meter_serial"))) {
            meter_serial = settings[F("meter_serial")].as<String>();
        }
        if (settings.containsKey(F("inverter_serial"))) {
            inverter_serial = settings[F("inverter_serial")].as<String>();
        }
        if (settings.containsKey(F("threshold"))) {
            threshold = settings[F("threshold")].as<uint32_t>();
        }
        initPowercontrol();
    }

private:
    String inverter_serial;
    String meter_serial;
    uint32_t threshold;
    powercontrolstruct powercontrol;
};

#endif /*__POWERCONTROLPLUGIN_H__*/