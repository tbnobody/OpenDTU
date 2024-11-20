#pragma once

#include <utility>
#include <vector>
#include <Arduino.h>

#include "JbdBmsDataPoints.h"

namespace JbdBms {

// Only valid for receiving messages
class SerialMessage {
    public:
        using tData = std::vector<uint8_t>;

        SerialMessage() = delete;

        enum class Command : uint8_t {
            Init = 0x00,
            ReadBasicInformation = 0x03,
            ReadCellVoltages = 0x04,
            ReadHardwareVersionNumber = 0x05,
            ControlMosInstruction = 0xE1,
        };

        uint8_t getStartMarker() const { return _raw[0]; }
        virtual Command getCommand() const = 0;
        uint8_t getDataLength() const { return _raw[3]; }
        uint16_t getChecksum() const { return get<uint16_t>(_raw.cend()-3); }
        uint8_t getEndMarker() const { return *(_raw.cend()-1); }
        void printMessage();

        bool isValid() const;

        uint8_t const* data() { return _raw.data(); }
        size_t size() { return _raw.size(); }

        static constexpr uint8_t startMarker = 0xDD;
        static constexpr uint8_t endMarker = 0x77;

    protected:
        template <typename... Args>
        explicit SerialMessage(Args&&... args) : _raw(std::forward<Args>(args)...) { }

        template<typename T, typename It> T get(It&& pos) const;
        template<typename It> bool getBool(It&& pos) const;
        template<typename It> int16_t getTemperature(It&& pos) const;
        template<typename It> std::string getString(It&& pos, size_t len, bool replaceZeroes = false) const;
        template<typename It> std::string getProductionDate(It&& pos) const;
        template<typename T> void set(tData::iterator const& pos, T val);
        uint16_t calcChecksum() const;
        void updateChecksum();

        tData _raw;
        JbdBms::DataPointContainer _dp;
};

class SerialResponse : public SerialMessage {
    public:
        enum class Status : uint8_t {
            Ok = 0x00,
            Error = 0x80
        };

        using tData = SerialMessage::tData;
        explicit SerialResponse(tData&& raw);

        Command getCommand() const { return static_cast<Command>(_raw[1]); }
        Status getStatus() const { return static_cast<Status>(_raw[2]); }

        bool isValid() const;

        DataPointContainer const& getDataPoints() const { return _dp; }
};

class SerialCommand : public SerialMessage {
    public:
        enum class Status : uint8_t {
            Read = 0xA5,
            Write = 0x5A,
        };

        explicit SerialCommand(Status status, Command cmd);

        Status getStatus() const { return static_cast<Status>(_raw[1]); }
        Command getCommand() const { return static_cast<Command>(_raw[2]); }
        static Command getLastCommand() { return _lastCmd; }

        bool isValid() const;

    private:
        static Command _lastCmd;
};


} /* namespace JbdBms */
