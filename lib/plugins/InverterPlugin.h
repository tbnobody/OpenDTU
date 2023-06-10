#ifndef __INVERTERPLUGIN_H__
#define __INVERTERPLUGIN_H__

#include "plugin.h"
#ifndef MAX_NUM_INVERTERS
#define MAX_NUM_INVERTERS 5
#endif

typedef struct {
    uint64_t serial = 0;
    String serialString;
    float actpower = 0;
    float actlimit = 0.0;
    float newlimit = 0.0;
    unsigned long limitTs = 0;
    bool updateInverter = false;
    bool updateLimit = false;
} inverterstruct;

class InverterPlugin : public Plugin {
    enum pluginIds { ACPOWER_INVERTER,
        ACPOWER_INVERTERSTRING,
        ACPOWER_PRODUCTION,
        ACPOWER_PRODUCTION_TOTAL };

public:
    InverterPlugin()
        : Plugin(1, "inverter")
    {
        inverterstruct def;
        inverters.fill(def);
    }

    void setup() { }
    void loop()
    {
        for (int i = 0; i < MAX_NUM_INVERTERS; i++) {
            if (inverters[i].updateInverter) {
                inverters[i].updateInverter = false;
                publishAC(inverters[i]);
            }
            if (inverters[i].updateLimit) {
                inverters[i].updateLimit = false;
                calcLimit(inverters[i]);
            }
        }
    }

    uint8_t getIndexBySerial(uint64_t serial)
    {
        for (int i = 0; i < MAX_NUM_INVERTERS; i++) {
            if (inverters[i].serial == serial) {
                return i;
            }
        }
        return MAX_NUM_INVERTERS;
    }

    uint8_t getIndexBySerialString(String& serial)
    {
        for (int i = 0; i < MAX_NUM_INVERTERS; i++) {
            if (serial.equals(inverters[i].serialString)) {
                return i;
            }
        }
        return MAX_NUM_INVERTERS;
    }

    uint8_t addInverter(uint64_t serial, const String& serialString)
    {
        for (int i = 0; i < MAX_NUM_INVERTERS; i++) {
            if (inverters[i].serial == 0) {
                inverters[i].serial = serial;
                inverters[i].serialString = serialString;
                return i;
            }
        }
        return MAX_NUM_INVERTERS;
    }

    void onTickerSetup()
    {
    }
    void setInverterLimit(uint8_t idx, float limit) {
        if(limit==-1) {
            MessageOutput.printf("inverterplugin: inverter[%d] limit -1 ignored\n",idx);
            return;
        }
        inverters[idx].newlimit = limit;
        inverters[idx].updateLimit = true;
    }
    void setInverterPower(uint8_t idx, float power) {
        inverters[idx].actpower = power;
        inverters[idx].updateInverter = true;
    }
    void setLimit(inverterstruct inverter)
    {
        auto inv = Hoymiles.getInverterBySerial(inverter.serial);
        if (inv != nullptr) {
            MessageOutput.printf("inverterplugin: sendActivePowerControlRequest %f W to %s\n", inverter.newlimit, inverter.serialString.c_str());
            if (inv->sendActivePowerControlRequest(inverter.newlimit, PowerLimitControlType::AbsolutNonPersistent)) {
                inverter.actlimit = inverter.newlimit;
                inverter.limitTs = millis();
            }
        }
    }

    void calcLimit(inverterstruct inverter)
    {
        MessageOutput.printf("inverterplugin: calcLimit[%s]\n", inverter.serialString.c_str());
        setLimit(inverter);
    }

    void publishAC(inverterstruct inverter)
    {
        MessageOutput.printf("inverterplugin: publishAC[%s]: %f\n", inverter.serialString.c_str(), inverter.actpower);
        PluginMessage m(*this);
        m.add(FloatValue(ACPOWER_PRODUCTION, inverter.actpower));
        m.add(LongValue(ACPOWER_INVERTER, inverter.serial));
        m.add(StringValue(ACPOWER_INVERTERSTRING, inverter.serialString));
        publishMessage(m);
    }
    void inverterCallback(const InverterMessage* message)
    {
        if (message->fieldId == FieldId_t::FLD_PAC && message->channelNumber == ChannelNum_t::CH0) {
            MessageOutput.printf("inverterplugin: new ac power: %f\n", message->value);
            uint8_t index = getIndexBySerial(message->inverterSerial);
            if (index == MAX_NUM_INVERTERS) {
                index = addInverter(message->inverterSerial, message->inverterStringSerial);
            }
            if (index == MAX_NUM_INVERTERS) {
                MessageOutput.printf("InverterPlugin: warning! no inverter with serial %s found\n", message->inverterStringSerial.c_str());
                return;
            }
            MessageOutput.printf("inverterplugin: update inverter.acpower[%s]: %f\n", message->inverterStringSerial.c_str(), message->value);
            setInverterPower(index,message->value);
        }
    }
    void internalDataCallback(PluginMessage* message)
    {
        if (message->from(PluginIds::PluginPowercontrol)) {
            float limit = -1;
            if (message->hasDataId(PluginPowercontrolIds::POWERLIMIT))
                limit = message->getDataAs<FloatValue>(PluginPowercontrolIds::POWERLIMIT).value;
            if (message->hasDataId(PluginPowercontrolIds::INVERTERSTRING)) {
                String sserial = message->getDataAs<StringValue>(PluginPowercontrolIds::INVERTERSTRING).value;
                uint8_t idx = getIndexBySerialString(sserial);
                if (idx != MAX_NUM_INVERTERS) {
                    MessageOutput.printf("InverterPlugin: PluginPowercontrol: found inverter with serialString %s\n", sserial.c_str());
                    setInverterLimit(idx,limit);
                }
            } else if (message->hasDataId(PluginPowercontrolIds::INVERTER)) {
                uint64_t serial = message->getDataAs<LongValue>(PluginPowercontrolIds::INVERTER).value;
                uint8_t idx = getIndexBySerial(serial);
                if (idx != MAX_NUM_INVERTERS) {
                    MessageOutput.printf("InverterPlugin: PluginPowercontrol: found inverter with serial %llu\n", serial);
                    setInverterLimit(idx,limit);
                }
            } else {
                MessageOutput.println("InverterPlugin: PluginPowercontrol: no inverter info found in message");
            }
        }
    }

private:
    std::array<inverterstruct, MAX_NUM_INVERTERS> inverters;
};

#endif /*__INVERTERPLUGIN_H__*/