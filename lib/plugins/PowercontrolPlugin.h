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

template <std::size_t N>
class powercontrollerarray : public structarray<powercontrolstruct, N> {
public:
    powercontrollerarray()
        : structarray<powercontrolstruct, N>()
    {
    }
    powercontrolstruct* getInverterByStringSerial(String& serial)
    {
        return structarray<powercontrolstruct, N>::getByKey([&serial](powercontrolstruct& pc) { return serial.equals(pc.inverterSerialString); });
    }
    powercontrolstruct* getInverterByLongSerial(uint64_t serial)
    {
        return structarray<powercontrolstruct, N>::getByKey([&serial](powercontrolstruct& pc) { return (serial == pc.inverterSerial); });
    }
    powercontrolstruct* getMeterByStringSerial(String& serial)
    {
        return structarray<powercontrolstruct, N>::getByKey([&serial](powercontrolstruct& pc) { return serial.equals(pc.meterSerial); });
    }
    powercontrolstruct* getEmptyIndex()
    {
        return structarray<powercontrolstruct, N>::getByKey([](powercontrolstruct& s) { return s.inverterSerialString.isEmpty(); });
    }
};

class PowercontrolAlgo {
public:
    PowercontrolAlgo() { }
    virtual bool calcLimit(powercontrolstruct& powercontrol) { return false; };
};

class DefaultPowercontrolAlgo : public PowercontrolAlgo {
public:
    DefaultPowercontrolAlgo()
        : PowercontrolAlgo()
    {
    }
    bool calcLimit(powercontrolstruct& powercontrol)
    {
        MessageOutput.printf("powercontrol PowercontrolAlgo: consumption=%f production=%f limit=%f\n", powercontrol.consumption, powercontrol.production, powercontrol.limit);
        // TODO: do some magic calculation here
        // :/
        // float newlimit = magicFunction(powercontrol.production,powercontrol.consumption);

        float newLimit = powercontrol.consumption;
        float threshold = std::abs(powercontrol.limit - newLimit);
        if (threshold <= powercontrol.threshold) {
            MessageOutput.printf("powercontrol PowercontrolAlgo: newlimit(%f) within threshold(%f) -> no limit change\n", newLimit, threshold);
        } else {
            MessageOutput.printf("powercontrol PowercontrolAlgo: setting limit to %f\n", newLimit);
            powercontrol.limit = newLimit;
            return true;
        }
        return false;
    }
};

class PowercontrolPlugin : public Plugin {
    enum pluginIds { INVERTER,
        INVERTERSTRING,
        POWERLIMIT };

public:
    PowercontrolPlugin()
        : Plugin(3, "powercontrol")
    {
    }

    void setup() { }
    void loop()
    {
        for (int i = 0; i < powercontrollers.size(); i++) {
            if (powercontrollers[i].update) {
                powercontrollers[i].update = false;
                if (calcLimit(powercontrollers[i])) {
                    publishLimit(powercontrollers[i]);
                }
            }
        }
    }

    bool calcLimit(powercontrolstruct& powercontrol)
    {
        return algo->calcLimit(powercontrol);
    }

    void publishLimit(powercontrolstruct& pc)
    {
        PluginMessage m(*this);
        m.add(StringValue(INVERTERSTRING, pc.inverterSerialString));
        m.add(FloatValue(POWERLIMIT, pc.limit));
        publishMessage(m);
        char topic[pc.inverterSerialString.length() + 7];
        char payload[32];
        snprintf(payload, sizeof(payload), "%f", pc.limit);
        snprintf(topic, sizeof(payload), "%s/updateLimit", pc.inverterSerialString.c_str());
        enqueueMessage(topic, payload, true);
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
            powercontrolstruct* powercontrol = powercontrollers.getMeterByStringSerial(meterserial);
            if (powercontrol) {
                powercontrol->consumption = message->getDataAs<FloatValue>(PluginMeterIds::METER_POWER).value;
                powercontrol->update = true;
                MessageOutput.printf("powercontrol got consumption: %f\n", powercontrol->consumption);
            } else {
                MessageOutput.printf("powercontrol meterserial(%s) not configured\n", meterserial.c_str());
            }
        } else if (message->has(PluginIds::PluginInverter, PluginInverterIds::ACPOWER_PRODUCTION)) {
            String inverterSerial = message->getDataAs<StringValue>(PluginInverterIds::ACPOWER_INVERTERSTRING).value;
            powercontrolstruct* powercontrol = powercontrollers.getInverterByStringSerial(inverterSerial);
            if (powercontrol) {
                powercontrol->production = message->getDataAs<FloatValue>(PluginInverterIds::ACPOWER_PRODUCTION).value;
                powercontrol->update = true;
                MessageOutput.printf("powercontrol got production: %f\n", powercontrol->production);
            } else {
                MessageOutput.printf("powercontrol inverterSerial(%s) not configured\n", inverterSerial.c_str());
            }
        } else {
            MessageOutput.printf("powercontrol unhandled message from sender=%d\n", message->getSenderId());
        }
    }
    void initPowercontrol()
    {
        powercontrolstruct* powercontrol = powercontrollers.getEmptyIndex();
        if (powercontrol) {
            powercontrol->inverterSerialString = inverter_serial;
            powercontrol->meterSerial = meter_serial;
            powercontrol->update = false;
            powercontrol->limit = 0;
            powercontrol->threshold = threshold;
        }
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
    uint32_t threshold = 20;
    // powercontrolstruct powercontrol;
    powercontrollerarray<MAX_NUM_INVERTERS> powercontrollers;
    DefaultPowercontrolAlgo defaultAlgo = DefaultPowercontrolAlgo();
    PowercontrolAlgo* algo = &defaultAlgo;
};

#endif /*__POWERCONTROLPLUGIN_H__*/