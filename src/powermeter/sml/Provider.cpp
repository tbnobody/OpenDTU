// SPDX-License-Identifier: GPL-2.0-or-later
#include <powermeter/sml/Provider.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "powerMeter";
#define SUBTAG _user

namespace PowerMeters::Sml {

void Provider::reset()
{
    smlReset();
    _dataInFlight.clear();
}

void Provider::processSmlByte(uint8_t byte)
{
    switch (smlState(byte)) {
        case SML_LISTEND:
            for (auto& handler: smlHandlerList) {
                if (!smlOBISCheck(handler.OBIS)) { continue; }

                float helper = 0.0;
                handler.decoder(helper);

                DTU_LOGD("decoded %s to %.2f", handler.name, helper);

                switch (handler.target)
                {
                case DataPointLabel::PowerTotal:
                    _dataInFlight.add<DataPointLabel::PowerTotal>(helper);
                    break;

                case DataPointLabel::PowerL1:
                    _dataInFlight.add<DataPointLabel::PowerL1>(helper);
                    break;

                case DataPointLabel::PowerL2:
                    _dataInFlight.add<DataPointLabel::PowerL2>(helper);
                    break;

                case DataPointLabel::PowerL3:
                    _dataInFlight.add<DataPointLabel::PowerL3>(helper);
                    break;

                case DataPointLabel::VoltageL1:
                    _dataInFlight.add<DataPointLabel::VoltageL1>(helper);
                    break;

                case DataPointLabel::VoltageL2:
                    _dataInFlight.add<DataPointLabel::VoltageL2>(helper);
                    break;

                case DataPointLabel::VoltageL3:
                    _dataInFlight.add<DataPointLabel::VoltageL3>(helper);
                    break;

                case DataPointLabel::CurrentL1:
                    _dataInFlight.add<DataPointLabel::CurrentL1>(helper);
                    break;

                case DataPointLabel::CurrentL2:
                    _dataInFlight.add<DataPointLabel::CurrentL2>(helper);
                    break;

                case DataPointLabel::CurrentL3:
                    _dataInFlight.add<DataPointLabel::CurrentL3>(helper);
                    break;

                case DataPointLabel::Import:
                    _dataInFlight.add<DataPointLabel::Import>(helper);
                    break;

                case DataPointLabel::Export:
                    _dataInFlight.add<DataPointLabel::Export>(helper);
                    break;

                default:
                    break;
                }
            }
            break;
        case SML_FINAL:
            _dataCurrent.updateFrom(_dataInFlight);
            reset();
            DTU_LOGD("TotalPower: %5.2f", getPowerTotal());
            break;
        case SML_CHECKSUM_ERROR:
            reset();
            DTU_LOGE("checksum verification failed");
            break;
        default:
            break;
    }
}

} // namespace PowerMeters::Sml
