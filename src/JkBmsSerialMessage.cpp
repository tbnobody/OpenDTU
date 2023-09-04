#include <numeric>

#include "JkBmsSerialMessage.h"
#include "MessageOutput.h"

namespace JkBms {

SerialCommand::SerialCommand(SerialCommand::Command cmd)
    : SerialMessage(20, 0x00)
{
    set(_raw.begin(), startMarker);
    set(_raw.begin() + 2, static_cast<uint16_t>(_raw.size() - 2)); // frame length
    set(_raw.begin() + 8, static_cast<uint8_t>(cmd));
    set(_raw.begin() + 9, static_cast<uint8_t>(Source::Host));
    set(_raw.begin() + 10, static_cast<uint8_t>(Type::Command));
    set(_raw.end() - 5, endMarker);
    updateChecksum();
}

using Label = JkBms::DataPointLabel;
template<Label L> using Traits = DataPointLabelTraits<L>;

SerialResponse::SerialResponse(tData&& raw, uint8_t protocolVersion)
    : SerialMessage(std::move(raw))
{
    if (!isValid()) { return; }

    auto pos = _raw.cbegin() + 11;
    auto end = pos + getVariableFieldLength();

    while ( pos < end ) {
        uint8_t fieldType = *(pos++);

        /**
         * there seems to be no way to make this more generic. the main reason
         * is that a non-constexpr value (fieldType cast as Label) cannot be
         * used as a template parameter.
         */
        switch(fieldType) {
            case 0x79:
            {
                uint8_t cellAmount = *(pos++) / 3;
                std::map<uint8_t, uint16_t> voltages;
                for (size_t cellCounter = 0; cellCounter < cellAmount; ++cellCounter) {
                    uint8_t idx = *(pos++);
                    auto cellMilliVolt = get<uint16_t>(pos);
                    voltages[idx] = cellMilliVolt;
                }
                _dp.add<Label::CellsMilliVolt>(voltages);
                break;
            }
            case 0x80:
                _dp.add<Label::BmsTempCelsius>(getTemperature(pos));
                break;
            case 0x81:
                _dp.add<Label::BatteryTempOneCelsius>(getTemperature(pos));
                break;
            case 0x82:
                _dp.add<Label::BatteryTempTwoCelsius>(getTemperature(pos));
                break;
            case 0x83:
                _dp.add<Label::BatteryVoltageMilliVolt>(static_cast<uint32_t>(get<uint16_t>(pos)) * 10);
                break;
            case 0x84:
                processBatteryCurrent(pos, protocolVersion);
                break;
            case 0x85:
                _dp.add<Label::BatterySoCPercent>(get<uint8_t>(pos));
                break;
            case 0x86:
                _dp.add<Label::BatteryTemperatureSensorAmount>(get<uint8_t>(pos));
                break;
            case 0x87:
                _dp.add<Label::BatteryCycles>(get<uint16_t>(pos));
                break;
            case 0x89:
                _dp.add<Label::BatteryCycleCapacity>(get<uint32_t>(pos));
                break;
            case 0x8a:
                _dp.add<Label::BatteryCellAmount>(get<uint16_t>(pos));
                break;
            case 0x8b:
                _dp.add<Label::AlarmsBitmask>(get<uint16_t>(pos));
                break;
            case 0x8c:
                _dp.add<Label::StatusBitmask>(get<uint16_t>(pos));
                break;
            case 0x8e:
                _dp.add<Label::TotalOvervoltageThresholdMilliVolt>(static_cast<uint32_t>(get<uint16_t>(pos)) * 10);
                break;
            case 0x8f:
                _dp.add<Label::TotalUndervoltageThresholdMilliVolt>(static_cast<uint32_t>(get<uint16_t>(pos)) * 10);
                break;
            case 0x90:
                _dp.add<Label::CellOvervoltageThresholdMilliVolt>(get<uint16_t>(pos));
                break;
            case 0x91:
                _dp.add<Label::CellOvervoltageRecoveryMilliVolt>(get<uint16_t>(pos));
                break;
            case 0x92:
                _dp.add<Label::CellOvervoltageProtectionDelaySeconds>(get<uint16_t>(pos));
                break;
            case 0x93:
                _dp.add<Label::CellUndervoltageThresholdMilliVolt>(get<uint16_t>(pos));
                break;
            case 0x94:
                _dp.add<Label::CellUndervoltageRecoveryMilliVolt>(get<uint16_t>(pos));
                break;
            case 0x95:
                _dp.add<Label::CellUndervoltageProtectionDelaySeconds>(get<uint16_t>(pos));
                break;
            case 0x96:
                _dp.add<Label::CellVoltageDiffThresholdMilliVolt>(get<uint16_t>(pos));
                break;
            case 0x97:
                _dp.add<Label::DischargeOvercurrentThresholdAmperes>(get<uint16_t>(pos));
                break;
            case 0x98:
                _dp.add<Label::DischargeOvercurrentDelaySeconds>(get<uint16_t>(pos));
                break;
            case 0x99:
                _dp.add<Label::ChargeOvercurrentThresholdAmps>(get<uint16_t>(pos));
                break;
            case 0x9a:
                _dp.add<Label::ChargeOvercurrentDelaySeconds>(get<uint16_t>(pos));
                break;
            case 0x9b:
                _dp.add<Label::BalanceCellVoltageThresholdMilliVolt>(get<uint16_t>(pos));
                break;
            case 0x9c:
                _dp.add<Label::BalanceVoltageDiffThresholdMilliVolt>(get<uint16_t>(pos));
                break;
            case 0x9d:
                _dp.add<Label::BalancingEnabled>(get<bool>(pos));
                break;
            case 0x9e:
                _dp.add<Label::BmsTempProtectionThresholdCelsius>(get<uint16_t>(pos));
                break;
            case 0x9f:
                _dp.add<Label::BmsTempRecoveryThresholdCelsius>(get<uint16_t>(pos));
                break;
            case 0xa0:
                _dp.add<Label::BatteryTempProtectionThresholdCelsius>(get<uint16_t>(pos));
                break;
            case 0xa1:
                _dp.add<Label::BatteryTempRecoveryThresholdCelsius>(get<uint16_t>(pos));
                break;
            case 0xa2:
                _dp.add<Label::BatteryTempDiffThresholdCelsius>(get<uint16_t>(pos));
                break;
            case 0xa3:
                _dp.add<Label::ChargeHighTempThresholdCelsius>(get<uint16_t>(pos));
                break;
            case 0xa4:
                _dp.add<Label::DischargeHighTempThresholdCelsius>(get<uint16_t>(pos));
                break;
            case 0xa5:
                _dp.add<Label::ChargeLowTempThresholdCelsius>(get<int16_t>(pos));
                break;
            case 0xa6:
                _dp.add<Label::ChargeLowTempRecoveryCelsius>(get<int16_t>(pos));
                break;
            case 0xa7:
                _dp.add<Label::DischargeLowTempThresholdCelsius>(get<int16_t>(pos));
                break;
            case 0xa8:
                _dp.add<Label::DischargeLowTempRecoveryCelsius>(get<int16_t>(pos));
                break;
            case 0xa9:
                _dp.add<Label::CellAmountSetting>(get<uint8_t>(pos));
                break;
            case 0xaa:
                _dp.add<Label::BatteryCapacitySettingAmpHours>(get<uint32_t>(pos));
                break;
            case 0xab:
                _dp.add<Label::BatteryChargeEnabled>(get<bool>(pos));
                break;
            case 0xac:
                _dp.add<Label::BatteryDischargeEnabled>(get<bool>(pos));
                break;
            case 0xad:
                _dp.add<Label::CurrentCalibrationMilliAmps>(get<uint16_t>(pos));
                break;
            case 0xae:
                _dp.add<Label::BmsAddress>(get<uint8_t>(pos));
                break;
            case 0xaf:
                _dp.add<Label::BatteryType>(get<uint8_t>(pos));
                break;
            case 0xb0:
                _dp.add<Label::SleepWaitTime>(get<uint16_t>(pos));
                break;
            case 0xb1:
                _dp.add<Label::LowCapacityAlarmThresholdPercent>(get<uint8_t>(pos));
                break;
            case 0xb2:
                _dp.add<Label::ModificationPassword>(getString(pos, 10));
                break;
            case 0xb3:
                _dp.add<Label::DedicatedChargerSwitch>(getBool(pos));
                break;
            case 0xb4:
                _dp.add<Label::EquipmentId>(getString(pos, 8));
                break;
            case 0xb5:
                _dp.add<Label::DateOfManufacturing >(getString(pos, 4));
                break;
            case 0xb6:
                _dp.add<Label::BmsHourMeterMinutes>(get<uint32_t>(pos));
                break;
            case 0xb7:
                _dp.add<Label::BmsSoftwareVersion>(getString(pos, 15));
                break;
            case 0xb8:
                _dp.add<Label::CurrentCalibration>(getBool(pos));
                break;
            case 0xb9:
                _dp.add<Label::ActualBatteryCapacityAmpHours>(get<uint32_t>(pos));
                break;
            case 0xba:
                _dp.add<Label::ProductId>(getString(pos, 24, true));
                break;
            case 0xc0:
                _dp.add<Label::ProtocolVersion>(get<uint8_t>(pos));
                break;
            default:
                MessageOutput.printf("unknown field type 0x%02x\r\n", fieldType);
                break;
        }
    }
}

/**
 * NOTE that this function moves the iterator by the amount of bytes read.
 */
template<typename T, typename It>
T SerialMessage::get(It&& pos) const
{
    // add easy-to-understand error message when called with non-const iter,
    // as compiler generated error message is hard to understand.
    using ItNoRef = typename std::remove_reference<It>::type;
    using PtrType = typename std::iterator_traits<ItNoRef>::pointer;
    using ValueType = typename std::remove_pointer<PtrType>::type;
    static_assert(std::is_const<ValueType>::value, "get() must be called with a const_iterator");

    // avoid out-of-bound read
    if (std::distance(pos, _raw.cend()) < sizeof(T)) { return 0; }

    T res = 0;
    for (unsigned i = 0; i < sizeof(T); ++i) {
        res |= static_cast<T>(*(pos++)) << (sizeof(T)-1-i)*8;
    }
    return res;
}

template<typename It>
bool SerialMessage::getBool(It&& pos) const
{
    uint8_t raw = get<uint8_t>(pos);
    return raw > 0;
}

template<typename It>
int16_t SerialMessage::getTemperature(It&& pos) const
{
    uint16_t raw = get<uint16_t>(pos);
    if (raw <= 100) { return static_cast<int16_t>(raw); }
    return static_cast<int16_t>(raw - 100) * (-1);
}

template<typename It>
std::string SerialMessage::getString(It&& pos, size_t len, bool replaceZeroes) const
{
    // avoid out-of-bound read
    len = std::min<size_t>(std::distance(pos, _raw.cend()), len);

    auto start = pos;
    pos += len;

    if (replaceZeroes) {
        std::vector<uint8_t> copy(start, pos);
        for (auto& c : copy) {
            if (c == 0) { c = 0x20; } // replace by ASCII space
        }
        return std::string(copy.cbegin(), copy.cend());
    }

    return std::string(start, pos);
}

void SerialMessage::processBatteryCurrent(SerialMessage::tData::const_iterator& pos, uint8_t protocolVersion)
{
    uint16_t raw = get<uint16_t>(pos);

    if (0x00 == protocolVersion) {
        // untested!
        _dp.add<Label::BatteryCurrentMilliAmps>((static_cast<int32_t>(10000) - raw) * 10);
        return;
    }
    else if (0x01 == protocolVersion) {
        bool charging = (raw & 0x8000) > 0;
        _dp.add<Label::BatteryCurrentMilliAmps>(static_cast<int32_t>(raw & 0x7FFF) * (charging ? 10 : -10));
        return;
    }

    MessageOutput.println("cannot decode battery current field without knowing the protocol version");
}

template<typename T>
void SerialMessage::set(tData::iterator const& pos, T val)
{
    // avoid out-of-bound write
    if (std::distance(pos, _raw.end()) < sizeof(T)) { return; }

    for (unsigned i = 0; i < sizeof(T); ++i) {
        *(pos+i) = static_cast<uint8_t>(val >> (sizeof(T)-1-i)*8);
    }
}

uint16_t SerialMessage::calcChecksum() const
{
    return std::accumulate(_raw.cbegin(), _raw.cend()-4, 0);
}

void SerialMessage::updateChecksum()
{
    set(_raw.end()-2, calcChecksum());
}

bool SerialMessage::isValid() const {
    uint16_t const actualStartMarker = get<uint16_t>(_raw.cbegin());
    if (actualStartMarker != startMarker) {
        MessageOutput.printf("JkBms::SerialMessage: invalid start marker %04x, expected 0x%04x\r\n",
            actualStartMarker, startMarker);
        return false;
    }

    uint16_t const frameLength = get<uint16_t>(_raw.cbegin()+2);
    if (frameLength != _raw.size() - 2) {
        MessageOutput.printf("JkBms::SerialMessage: unexpected frame length %04x, expected 0x%04x\r\n",
            frameLength, _raw.size() - 2);
        return false;
    }

    uint8_t const actualEndMarker = *(_raw.cend()-5);
    if (actualEndMarker != endMarker) {
        MessageOutput.printf("JkBms::SerialMessage: invalid end marker %02x, expected 0x%02x\r\n",
            actualEndMarker, endMarker);
        return false;
    }

    uint16_t const actualChecksum = get<uint16_t>(_raw.cend()-2);
    uint16_t const expectedChecksum = calcChecksum();
    if (actualChecksum != expectedChecksum) {
        MessageOutput.printf("JkBms::SerialMessage: invalid checksum 0x%04x, expected 0x%04x\r\n",
            actualChecksum, expectedChecksum);
        return false;
    }

    return true;
}

} /* namespace JkBms */
