#ifndef __INVERTERPLUGIN_H__
#define __INVERTERPLUGIN_H__

#include "plugin.h"
#include <Hoymiles.h>

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

template <std::size_t N>
class inverterarray : public structarray<inverterstruct, N> {
public:
    inverterarray()
        : structarray<inverterstruct, N>()
    {
    }
    inverterstruct* getInverterByStringSerial(String& serial)
    {
        return structarray<inverterstruct, N>::getByKey([&serial](inverterstruct& m) { return serial.equals(m.serialString); });
    }
    inverterstruct* getInverterByLongSerial(uint64_t serial)
    {
        return structarray<inverterstruct, N>::getByKey([&serial](inverterstruct& m) { return (serial == m.serial); });
    }
    inverterstruct* getEmptyIndex()
    {
        return structarray<inverterstruct, N>::getByKey([](inverterstruct& s) { return s.serialString.isEmpty(); });
    }
};

class InverterPlugin : public Plugin {
    enum pluginIds { ACPOWER_INVERTER,
        ACPOWER_INVERTERSTRING,
        ACPOWER_PRODUCTION,
        ACPOWER_PRODUCTION_TOTAL };

public:
    InverterPlugin()
        : Plugin(1, "inverter")
    {
    }

    void setup() { }
    void loop()
    {
        for (int i = 0; i < inverters.size(); i++) {
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
    void onTickerSetup()
    {
        addTimerCb(
            SECOND, 5, [this]() { loopInverters(); }, "loopInvertersTimer");
    }

    void loopInverters()
    {
        // copied from WebApi_prometheus.cpp
        // TODO: polling is bad. 
        // notify on update would be fine!
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            MessageOutput.printf("inverterplugin: loopInverters inv[%d] lastupdate=%d\n", i, inv->Statistics()->getLastUpdate());
            if (inv->Statistics()->getLastUpdate() > 0) {
                for (auto& t : inv->Statistics()->getChannelTypes()) {
                    for (auto& c : inv->Statistics()->getChannelsByType(t)) {
                        publishField(inv, t, c, FLD_PAC);
                        publishField(inv, t, c, FLD_UAC);
                        publishField(inv, t, c, FLD_IAC);
                        publishField(inv, t, c, FLD_PDC);
                        publishField(inv, t, c, FLD_UDC);
                        publishField(inv, t, c, FLD_IDC);
                        publishField(inv, t, c, FLD_YD);
                        publishField(inv, t, c, FLD_YT);
                        publishField(inv, t, c, FLD_F);
                        publishField(inv, t, c, FLD_T);
                        publishField(inv, t, c, FLD_PF);
                        publishField(inv, t, c, FLD_Q);
                        publishField(inv, t, c, FLD_EFF);
                        publishField(inv, t, c, FLD_IRR);
                    }
                }
            }
        }
    }

    void publishField(std::shared_ptr<InverterAbstract> inv, ChannelType_t& type, ChannelNum_t& channel, FieldId_t fieldId)
    {
        // hack !!!!!
        if(fieldId != FieldId_t::FLD_PAC) {
            return;
        }
        if (inv->Statistics()->hasChannelFieldValue(type, channel, fieldId)) {
            String value = String(
                inv->Statistics()->getChannelFieldValue(type, channel, fieldId),
                static_cast<unsigned int>(inv->Statistics()->getChannelFieldDigits(type, channel, fieldId)));
            value.trim();
            InverterMessage message;
            message.inverterSerial = inv.get()->serial();
            message.inverterStringSerial = inv.get()->serialString();
            message.fieldId = fieldId;
            message.channelType = type;
            message.channelNumber = channel;
            message.value = value.toFloat();
            inverterCallback(&message);
        }
    }

    inverterstruct* addInverter(uint64_t serial, const String& serialString)
    {
        inverterstruct* inverter = inverters.getEmptyIndex();
        if (inverter != nullptr) {
            inverter->serial = serial;
            inverter->serialString = serialString;
        }
        return inverter;
    }

    void setInverterLimit(inverterstruct* inverter, float limit)
    {
        if (limit == -1) {
            MessageOutput.printf("inverterplugin: inverter[%s] limit -1 ignored\n", inverter->serialString.c_str());
            return;
        }
        inverter->newlimit = limit;
        inverter->updateLimit = true;
    }
    void setInverterPower(inverterstruct* inverter, float power)
    {
        inverter->actpower = power;
        inverter->updateInverter = true;
    }
    void setLimit(inverterstruct& inverter)
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

    void calcLimit(inverterstruct& inverter)
    {
        MessageOutput.printf("inverterplugin: calcLimit[%s]\n", inverter.serialString.c_str());
        setLimit(inverter);
    }

    void publishAC(inverterstruct& inverter)
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
            inverterstruct* index = inverters.getInverterByLongSerial(message->inverterSerial);
            if (index == nullptr) {
                index = addInverter(message->inverterSerial, message->inverterStringSerial);
            }
            if (index == nullptr) {
                MessageOutput.printf("InverterPlugin: warning! no inverter with serial %s found\n", message->inverterStringSerial.c_str());
                return;
            }
            MessageOutput.printf("inverterplugin: update inverter.acpower[%s]: %f\n", message->inverterStringSerial.c_str(), message->value);
            setInverterPower(index, message->value);
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
                inverterstruct* inverter = inverters.getInverterByStringSerial(sserial);
                if (inverter != nullptr) {
                    MessageOutput.printf("InverterPlugin: PluginPowercontrol: found inverter with serialString %s\n", sserial.c_str());
                    setInverterLimit(inverter, limit);
                }
            } else if (message->hasDataId(PluginPowercontrolIds::INVERTER)) {
                uint64_t serial = message->getDataAs<LongValue>(PluginPowercontrolIds::INVERTER).value;
                inverterstruct* inverter = inverters.getInverterByLongSerial(serial);
                if (inverter != nullptr) {
                    MessageOutput.printf("InverterPlugin: PluginPowercontrol: found inverter with serial %llu\n", serial);
                    setInverterLimit(inverter, limit);
                }
            } else {
                MessageOutput.println("InverterPlugin: PluginPowercontrol: no inverter info found in message");
            }
        }
    }

private:
    inverterarray<MAX_NUM_INVERTERS> inverters;
};

#endif /*__INVERTERPLUGIN_H__*/