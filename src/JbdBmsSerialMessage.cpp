#include <numeric>
#include <sstream>
#include <iomanip>
#include "JbdBmsSerialMessage.h"
#include "MessageOutput.h"

namespace JbdBms {

SerialCommand::SerialCommand(SerialCommand::Status status, SerialCommand::Command cmd)
    : SerialMessage(7, 0x00) // frame length 7 bytes initialized with zeros
{
    set(_raw.begin(), startMarker);
    set(_raw.begin() + 1, static_cast<uint8_t>(status));
    set(_raw.begin() + 2, static_cast<uint8_t>(cmd));
    set(_raw.begin() + 3, static_cast<uint16_t>(0x00)); // frame length
    updateChecksum();
    set(_raw.end() - 1, endMarker);

    _lastCmd = cmd;
}

SerialCommand::Command SerialCommand::_lastCmd = SerialCommand::Command::Init;


using Label = JbdBms::DataPointLabel;
template<Label L> using Traits = DataPointLabelTraits<L>;

SerialResponse::SerialResponse(tData&& raw)
    : SerialMessage(std::move(raw))
{
    if (!isValid()) { return; }

    auto pos = _raw.cbegin() + 4; // start of data content
    auto end = pos + getDataLength(); // end of data content

    if (pos < end) {

        if (getCommand() == Command::ReadBasicInformation) {

            _dp.add<Label::BatteryVoltageMilliVolt>(static_cast<uint32_t>(get<uint16_t>(pos)) * 10); // Total voltage
            _dp.add<Label::BatteryCurrentMilliAmps>(static_cast<int32_t>(get<int16_t>(pos)) * 10); // Current
            _dp.add<Label::ActualBatteryCapacityAmpHours>(static_cast<uint32_t>(get<uint16_t>(pos)) * 10 / 1000); // remaining capacity
            _dp.add<Label::BatteryCapacitySettingAmpHours>(static_cast<uint32_t>(get<uint16_t>(pos)) * 10 / 1000); // nominal capacity
            _dp.add<Label::BatteryCycles>(get<uint16_t>(pos));
            _dp.add<Label::DateOfManufacturing >(getProductionDate(pos));

            bool balancingEnabled = false;
            balancingEnabled |= static_cast<bool>(get<uint16_t>(pos)); // Equilibrium
            balancingEnabled |= static_cast<bool>(get<uint16_t>(pos)); // Equilibrium_High
            _dp.add<Label::BalancingEnabled>(balancingEnabled);

            _dp.add<Label::AlarmsBitmask>(get<uint16_t>(pos)); // Protection status

            uint8_t softwareVersion = get<uint8_t>(pos);
            uint8_t digitOne = softwareVersion & 0x0F;
            uint8_t digitTwo = softwareVersion >> 4;
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%d.%d", digitOne, digitTwo);
            _dp.add<Label::BmsSoftwareVersion>(std::string(buffer)); // Software version

            _dp.add<Label::BatterySoCPercent>(get<uint8_t>(pos)); // RSOC

            uint8_t fetControl = get<uint8_t>(pos); // FET control status
            const uint8_t chargingMask = (1 << 0);
            const uint8_t dischargingMask = (1 << 1);
            bool fetChargeEnabled = static_cast<bool>(fetControl & chargingMask);
            bool fetDischargeEnabled = static_cast<bool>(fetControl & dischargingMask);
            _dp.add<Label::BatteryChargeEnabled>(fetChargeEnabled);
            _dp.add<Label::BatteryDischargeEnabled>(fetDischargeEnabled);

            _dp.add<Label::BatteryCellAmount>(static_cast<uint16_t>(get<uint8_t>(pos))); // number of battery strings
            _dp.add<Label::BatteryTemperatureSensorAmount>(get<uint8_t>(pos)); // number of ntc
            _dp.add<Label::BatteryTempOneCelsius>(getTemperature(pos)); // ntc temperature one
            _dp.add<Label::BatteryTempTwoCelsius>(getTemperature(pos)); // ntc temperature two
        }
        else if (getCommand() == Command::ReadCellVoltages)
        {
            uint8_t cellAmount = getDataLength() / 2;
            std::map<uint8_t, uint16_t> voltages;
            for (size_t cellCounter = 0; cellCounter < cellAmount; ++cellCounter) {
                uint8_t idx = cellCounter;
                auto cellMilliVolt = get<uint16_t>(pos);
                voltages[idx] = cellMilliVolt;
            }
            _dp.add<Label::CellsMilliVolt>(voltages);
        }
        else if (getCommand() == Command::ReadHardwareVersionNumber)
        {
            _dp.add<Label::BmsHardwareVersion>(getString(pos, getDataLength()));
        }
        else if (getCommand() == Command::ControlMosInstruction)
        {
            /* Response doesn't contain any data content */
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
    // raw in 0.1K
    uint16_t raw = get<uint16_t>(pos);
    return static_cast<int16_t>(raw - 2731) / 10;
}

template<typename It>
std::string SerialMessage::getProductionDate(It&& pos) const
{
    // E.g. 0x2068 = 08.03.2016
    // the date is the lowest 5: 0x2028 & 0x1f = 8 means the date;
    // month (0x2068>>5) & 0x0f = 0x03 means March;
    // the year is 2000+ (0x2068>>9) = 2000 + 0x10 =2016;
    uint16_t raw = get<uint16_t>(pos);

    uint16_t day = raw & 0x1f;
    uint16_t month = (raw>>5) & 0x0f;
    uint16_t year = 2000 + (raw>>9);
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << day << "." << std::setw(2) << std::setfill('0') << month << "." << std::setw(4) << std::setfill('0') << year;
    return oss.str();
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
    return (~std::accumulate(_raw.cbegin()+2, _raw.cend()-3, 0) + 0x01);
}

void SerialMessage::updateChecksum()
{
    set(_raw.end()-3, calcChecksum());
}

void SerialMessage::printMessage() {

    double ts = static_cast<double>(millis())/1000;
    MessageOutput.printf("[%11.3f] JBD BMS: raw message (%d Bytes):",
        ts, _raw.size());
    for (size_t ctr = 0; ctr < _raw.size(); ++ctr) {
        MessageOutput.printf(" %02x", _raw[ctr]);
    }
    MessageOutput.println();
}

bool SerialMessage::isValid() const {

    uint8_t const actualStartMarker = getStartMarker();
    if (actualStartMarker != startMarker) {
        MessageOutput.printf("JbdBms::SerialMessage: invalid start marker 0x%02x, expected 0x%02x\r\n",
            actualStartMarker, startMarker);
        return false;
    }

    uint16_t const dataLength = getDataLength();
    uint16_t const dataLengthExpected = _raw.size() - 7;
    if (dataLength != _raw.size() - 7) {
        MessageOutput.printf("JbdBms::SerialMessage: unexpected data length 0x%04x, expected 0x%04x\r\n",
            dataLength, dataLengthExpected);
        return false;
    }

    uint8_t const actualEndMarker = getEndMarker();
    if (actualEndMarker != endMarker) {
        MessageOutput.printf("JbdBms::SerialMessage: invalid end marker 0x%02x, expected 0x%02x\r\n",
            actualEndMarker, endMarker);
        return false;
    }

    uint16_t const actualChecksum = getChecksum();
    uint16_t const expectedChecksum = calcChecksum();
    if (actualChecksum != expectedChecksum) {
        MessageOutput.printf("JbdBms::SerialMessage: invalid checksum 0x%04x, expected 0x%04x\r\n",
            actualChecksum, expectedChecksum);
        return false;
    }

    return true;
}

bool SerialResponse::isValid() const {

    if(!SerialMessage::isValid()) {return false;}

    Status const actualStatus = getStatus();
    if (actualStatus != Status::Ok) {
        MessageOutput.printf("JbdBms::SerialMessage: invalid status 0x%02x, expected 0x%02x\r\n",
            (uint32_t) actualStatus, (uint32_t) Status::Ok);
        return false;
    }

    return true;
}

bool SerialCommand::isValid() const {

    if(!SerialMessage::isValid()) {return false;}

    Status const actualStatus = getStatus();
    if (actualStatus != Status::Read || actualStatus != Status::Write) {
        MessageOutput.printf("JbdBms::SerialMessage: invalid status 0x%02x, expected 0x%02x or 0x%02x\r\n",
            (uint32_t) actualStatus, (uint32_t) Status::Read, (uint32_t) Status::Write);
        return false;
    }

    return true;
}

} /* namespace JbdBms */
