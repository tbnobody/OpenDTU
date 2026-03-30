#include <functional>
#include <Configuration.h>
#include <battery/zendure/Provider.h>
#include <MqttSettings.h>
#include <SunPosition.h>
#include <Utils.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "battery";
static const char* SUBTAG = "Zendure";

namespace Batteries::Zendure {

Provider::Provider()
    : _stats(std::make_shared<Stats>())
    , _hassIntegration(std::make_shared<HassIntegration>(_stats)) { }

bool Provider::init()
{
    auto const& config = Configuration.get();
    String deviceType = String();

    DTU_LOGD("Settings %" PRIu8, config.Battery.Zendure.DeviceType);
    {
        String deviceName = String();
        switch (config.Battery.Zendure.DeviceType) {
            case 0:
                deviceType = ZENDURE_HUB1200;
                deviceName = String("SolarFlow HUB 1200");
                _full_log_supported = true;
                break;
            case 1:
                deviceType = ZENDURE_HUB2000;
                deviceName = String("SolarFlow HUB 2000");
                _full_log_supported = true;
                break;
            case 2:
                deviceType = ZENDURE_AIO2400;
                deviceName = String("AIO 2400");
                break;
            case 3:
                deviceType = ZENDURE_ACE1500;
                deviceName = String("SolarFlow Ace 1500");
                break;
            case 4:
                deviceType = ZENDURE_HYPER2000_A;
                deviceName = String("SolarFlow Hyper 2000");
                break;
            case 5:
                deviceType = ZENDURE_HYPER2000_B;
                deviceName = String("SolarFlow Hyper 2000");
                break;
            default:
                DTU_LOGE("Invalid device type!");
                return false;
        }

        if (strlen(config.Battery.Zendure.DeviceId) != 8) {
            DTU_LOGE("Invalid device id '%s'!", config.Battery.Zendure.DeviceId);
            return false;
        }

        // setup static device info
        DTU_LOGI("Device name '%s' - LOG messages are %s supported\r\n",
                deviceName.c_str(), (_full_log_supported ? "fully" : "partly"));
        _stats->setDevice(std::move(deviceName));
        _stats->setManufacturer("Zendure");
    }

    // store device ID as we will need them for checking when receiving messages
    setTopics(deviceType, config.Battery.Zendure.DeviceId);

    _topicPersistentSettings = MqttSettings.getPrefix() + "battery/persistent/";

    auto topic = _topicPersistentSettings + "#";
    MqttSettings.subscribe(topic, 0/*QoS*/,
            std::bind(&Provider::onMqttMessagePersistentSettings,
                this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4)
            );
    DTU_LOGD("Subscribed to '%s' for persistent settings", topic.c_str());

    // subscribe for log messages
    MqttSettings.subscribe(_topicLog, 0/*QoS*/,
            std::bind(&Provider::onMqttMessageLog,
                this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4)
            );
    DTU_LOGD("Subscribed to '%s' for status readings", _topicLog.c_str());

    // subscribe for report messages
    MqttSettings.subscribe(_topicReport, 0/*QoS*/,
            std::bind(&Provider::onMqttMessageReport,
                this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4)
            );
    DTU_LOGD("Subscribed to '%s' for status readings", _topicReport.c_str());

    // subscribe for timesync messages
    MqttSettings.subscribe(_topicTimesync, 0/*QoS*/,
            std::bind(&Provider::onMqttMessageTimesync,
                this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4)
            );
    DTU_LOGD("Subscribed to '%s' for timesync requests", _topicTimesync.c_str());

    _rateFullUpdateMs   = config.Battery.Zendure.PollingInterval * 1000;
    _nextFullUpdate     = millis() + _rateFullUpdateMs / 2;
    _rateTimesyncMs     = ZENDURE_SECONDS_TIMESYNC * 1000;
    _nextTimesync       = _nextFullUpdate;
    _rateSunCalcMs      = ZENDURE_SECONDS_SUNPOSITION * 1000;
    _nextSunCalc        = millis() + _rateSunCalcMs / 2;


    // pre-generate the full update request
    JsonDocument root;
    JsonArray array = root[ZENDURE_REPORT_PROPERTIES].to<JsonArray>();
    array.add("getAll");
    array.add("getInfo");
    serializeJson(root, _payloadFullUpdate);

    // disable charge through cycle if disable by config
    if (!config.Battery.Zendure.ChargeThroughEnable) {
        setChargeThroughState(ChargeThroughState::Disabled);
    }

    // check if we are allowed to write stuff
    if (config.Battery.Zendure.ControlMode == BatteryZendureConfig::ControlMode::ControlModeReadOnly) {
        DTU_LOGI("Running in READ-ONLY mode");

        // forget about write topic to prevent it will ever be written
        _topicWrite.clear();
    }

    DTU_LOGI("INIT DONE");
    return true;
}

void Provider::setTopics(const String& deviceType, const String& deviceId) {
    String baseTopic = "/" + deviceType + "/" + deviceId + "/";

    _topicRead   = "iot" + baseTopic + "properties/read";
    _topicWrite  = "iot" + baseTopic + "properties/write";
    _topicLog    = baseTopic + "log";
    _topicReport = baseTopic + "properties/report";

    _topicTimesync      = baseTopic + "time-sync";
    _topicTimesyncReply = "iot" + baseTopic + "time-sync/reply";
}

void Provider::deinit()
{
    if (!_topicReport.isEmpty()) {
        MqttSettings.unsubscribe(_topicReport);
        _topicReport.clear();
    }
    if (!_topicLog.isEmpty()) {
        MqttSettings.unsubscribe(_topicLog);
        _topicLog.clear();
    }
    if (!_topicTimesync.isEmpty()) {
        MqttSettings.unsubscribe(_topicTimesync);
        _topicTimesync.clear();
    }
    if (!_topicPersistentSettings.isEmpty()) {
        MqttSettings.unsubscribe(_topicPersistentSettings + "#");
        _topicPersistentSettings.clear();
    }
}

void Provider::loop()
{
    auto ms = millis();
    auto const& config = Configuration.get();
    const bool isDayPeriod = SunPosition.isSunsetAvailable() ? SunPosition.isDayPeriod() : true;
    auto const chargeThroughState = _stats->_charge_through_state.value_or(ChargeThroughState::Disabled);

    // if auto shutdown is enabled and battery switches to idle at night, turn off status requests to prevent keeping battery awake
    if (config.Battery.Zendure.AutoShutdown && !isDayPeriod && _stats->_state == State::Idle) {
        return;
    }

    // check if we run in schedule mode
    if (ms >= _nextSunCalc) {
        _nextSunCalc = ms + _rateSunCalcMs;

        calculateFullChargeAge();

        struct tm timeinfo_local;
        struct tm timeinfo_sun;
        if (getLocalTime(&timeinfo_local, 5)) {
            std::time_t current = std::mktime(&timeinfo_local);

            std::time_t sunrise = 0;
            std::time_t sunset = 0;

            if (SunPosition.sunriseTime(&timeinfo_sun)) {
                sunrise = std::mktime(&timeinfo_sun) + config.Battery.Zendure.SunriseOffset * 60;
            }

            if (SunPosition.sunsetTime(&timeinfo_sun)) {
                sunset = std::mktime(&timeinfo_sun) + config.Battery.Zendure.SunsetOffset * 60;
            }

            if (sunrise && sunset) {
                // check charge-through at sunrise (make sure its triggered at least once)
                if (current > sunrise && current < (sunrise + ZENDURE_SECONDS_SUNPOSITION + ZENDURE_SECONDS_SUNPOSITION/2)) {
                    // Calculate expected daylight to asure charge through starts in the morning if sheduled for this day
                    // We just use the time between rise and set, as we do not know anything about the actual conditions,
                    // we can only expect that there will be NO sun between sunset and sunrise ;)
                    uint32_t maxDaylightHours = (sunset - sunrise + 1800U) / 3600U;
                    checkChargeThrough(maxDaylightHours);
                }

                // running in appointment mode - set outputlimit accordingly
                if (config.Battery.Zendure.OutputControl == BatteryZendureConfig::OutputControl_t::ControlSchedule && chargeThroughState != ChargeThroughState::Hard) {
                    if (current >= sunrise && current < sunset) {
                        setOutputLimit(min(config.Battery.Zendure.MaxOutput, config.Battery.Zendure.OutputLimitDay));
                    } else if (current >= sunset || current < sunrise) {
                        setOutputLimit(min(config.Battery.Zendure.MaxOutput, config.Battery.Zendure.OutputLimitNight));
                    }
                }
            }


        }

        // ensure charge through settings
        switch (chargeThroughState) {
            case ChargeThroughState::Soft:
            case ChargeThroughState::Keep:
                setTargetSoCs(config.Battery.Zendure.MinSoC, 100);
                setBypassMode(BatteryZendureConfig::BypassMode_t::AlwaysOff);
                setOutputLimit(config.Battery.Zendure.OutputLimit);
                break;
            case ChargeThroughState::Hard:
                setTargetSoCs(config.Battery.Zendure.MinSoC, 100);
                setBypassMode(BatteryZendureConfig::BypassMode_t::AlwaysOff);
                setOutputLimit(0);
                break;
            default:
                setTargetSoCs(config.Battery.Zendure.MinSoC, config.Battery.Zendure.MaxSoC);
                setBypassMode(config.Battery.Zendure.BypassMode);
                setOutputLimit(config.Battery.Zendure.OutputLimit);
                break;
        }

    }

    if (!_topicRead.isEmpty()) {
        if (!_payloadFullUpdate.isEmpty() && ms >= _nextFullUpdate) {
            _nextFullUpdate = ms + _rateFullUpdateMs;
            MqttSettings.publishGeneric(_topicRead, _payloadFullUpdate, false, 0);
        }
    }

    if (ms >= _nextTimesync) {
        _nextTimesync = ms + _rateTimesyncMs;
        timesync();

        // update settings (will be skipped if unchanged)
        setInverterMax(config.Battery.Zendure.MaxOutput);

        // republish settings - just to be sure
        writeSettings();
    }
}

void Provider::writeSettings() {
    if (_topicWrite.isEmpty()) {
        return;
    }

    auto const& config = Configuration.get();

    setBuzzer(config.Battery.Zendure.BuzzerEnable);
    setAutoshutdown(config.Battery.Zendure.AutoShutdown);

    publishProperties(_topicWrite,
        ZENDURE_REPORT_PV_BRAND,        "1",    // means Hoymiles
        ZENDURE_REPORT_PV_AUTO_MODEL,   "0",    // we did static setup
        ZENDURE_REPORT_SMART_MODE,      "0"     // disable smart mode
    );

    // if running in OnlyOnce mode, forget about write topic to prevent it will ever be written again
    if (config.Battery.Zendure.ControlMode == BatteryZendureConfig::ControlMode::ControlModeOnce) {
        _topicWrite.clear();
    }
}

void Provider::calculateFullChargeAge()
{
    time_t now;
    if (!Utils::getEpoch(&now)) {
        return;
    }

    if(_stats->_last_full_timestamp.has_value()) {
        auto last_full = *(_stats->_last_full_timestamp);
        uint32_t age = now > last_full  ? (now - last_full) / 3600U : 0U;

        DTU_LOGD("Now: %ld, LastFull: %" PRIu64 ", Diff: %" PRIu32, now, last_full, age);

        // store for webview
        _stats->_last_full_hours = age;
    }

    if(_stats->_last_empty_timestamp.has_value()) {
        auto last_empty = *(_stats->_last_empty_timestamp);
        uint32_t age = now > last_empty  ? (now - last_empty) / 3600U : 0U;

        DTU_LOGD("Now: %ld, LastEmpty: %" PRIu64 ", Diff: %" PRIu32, now, last_empty, age);

        // store for webview
        _stats->_last_empty_hours = age;
    }

}

void Provider::checkChargeThrough(uint32_t predictHours /* = 0 */)
{
    auto const& config = Configuration.get();
    if (!config.Battery.Zendure.ChargeThroughEnable) {
        return;
    }

    // hard charge through will start after configured interval (given in hours)
    auto hardChargeThrough = config.Battery.Zendure.ChargeThroughInterval;

    // soft charge through will be triggered one day (aka. 24 hours) before hard charge through
    auto softChargeThrough = hardChargeThrough - 24;

    auto currentValue      = _stats->_last_full_hours.value_or(0) + predictHours;
    auto noValue           = !_stats->_last_full_timestamp.has_value();

    if (noValue || currentValue > hardChargeThrough) {
        return setChargeThroughState(ChargeThroughState::Hard);
    }

    if (noValue || currentValue > softChargeThrough) {
        return setChargeThroughState(ChargeThroughState::Soft);
    }

    // force IDLE state to prevent sticking at KEEP state
    setChargeThroughState(ChargeThroughState::Idle);
}

void Provider::setTargetSoCs(const float soc_min, const float soc_max)
{
    if (_topicWrite.isEmpty() || !alive()) {
        return;
    }

    if (_stats->_soc_min != soc_min || _stats->_soc_max != soc_max) {
        publishProperties(_topicWrite, ZENDURE_REPORT_MIN_SOC, String(soc_min * 10, 0), ZENDURE_REPORT_MAX_SOC, String(soc_max * 10, 0));
        DTU_LOGD("Setting target minSoC from %.1f %% to %.1f %% and target maxSoC from %.1f %% to %.1f %%",
                _stats->_soc_min, soc_min, _stats->_soc_max, soc_max);
    }
}

uint16_t Provider::calcOutputLimit(uint16_t limit) const
{
    if (limit >= 100 || limit == 0 ) {
        return limit;
    }

    uint16_t base = limit / 30U;
    uint16_t remain = (limit % 30U) / 15U;
    return 30 * base + 30 * remain;
}

uint16_t Provider::setOutputLimit(uint16_t limit) const
{
    auto const& config = Configuration.get();

    if (config.Battery.Zendure.OutputControl == BatteryZendureConfig::OutputControl_t::ControlNone ||
        _topicWrite.isEmpty() || !alive() ) {
        return _stats->_output_limit;
    }

    // keep limit below MaxOutput
    limit = min(config.Battery.Zendure.MaxOutput, limit);

    if (_stats->_output_limit != limit) {
        limit = calcOutputLimit(limit);
        publishProperty(_topicWrite, ZENDURE_REPORT_OUTPUT_LIMIT, String(limit));
        DTU_LOGD("Adjusting outputlimit from %" PRIu16 " W to %" PRIu16 " W", _stats->_output_limit, limit);
    }

    return limit;
}

void Provider::setBypassMode(BatteryZendureConfig::BypassMode_t mode) const
{
    if (_topicWrite.isEmpty() || !alive()) {
        return;
    }

    if (_stats->_bypass_mode != mode) {
        publishProperty(_topicWrite, ZENDURE_REPORT_BYPASS_MODE, String(static_cast<uint8_t>(mode)));
        DTU_LOGD("Adjusting bypassmode from %" PRIu8 " to %" PRIu8 "", _stats->_bypass_mode, mode);
    }

    bool recover = (mode == BatteryZendureConfig::BypassMode_t::Automatic);
    if (_stats->_auto_recover != recover) {
        publishProperty(_topicWrite, ZENDURE_REPORT_AUTO_RECOVER, String(static_cast<uint8_t>(recover)));
        DTU_LOGD("Adjusting autorecover from %" PRIu8 " to %" PRIu8 "", _stats->_auto_recover, recover);
    }
}

uint16_t Provider::setInverterMax(uint16_t limit) const
{
    if (_topicWrite.isEmpty() || !alive()) {
        return _stats->_inverse_max;
    }

    if (_stats->_inverse_max != limit) {
        limit = calcOutputLimit(limit);
        publishProperty(_topicWrite, ZENDURE_REPORT_INVERSE_MAX_POWER, String(limit));
        DTU_LOGD("Adjusting inverter max output from %" PRIu16 " W to %" PRIu16 " W", _stats->_inverse_max, limit);
    }

    return limit;
}

void Provider::setBuzzer(bool enable) const
{
    if (_topicWrite.isEmpty() || !alive() || _stats->_buzzer == enable) {
        return;
    }

    publishProperty(_topicWrite, ZENDURE_REPORT_BUZZER_SWITCH, String(enable ? "1" : "0"));
    DTU_LOGD("%s buzzer", enable ? "Enabling" : "Disabling");
}

void Provider::setAutoshutdown(bool enable) const
{
    if (_topicWrite.isEmpty() || !alive() || _stats->_auto_shutdown == enable) {
        return;
    }

    publishProperty(_topicWrite, ZENDURE_REPORT_AUTO_SHUTDOWN, String(enable ? "1" : "0"));
    DTU_LOGD("%s autoshutdown", enable ? "Enabling" : "Disabling");
}

void Provider::shutdown() const
{
    if (!_topicWrite.isEmpty()) {
        publishProperty(_topicWrite, ZENDURE_REPORT_MASTER_SWITCH, "1");
        DTU_LOGD("Shutting down HUB");
    }
}

void Provider::publishProperty(const String& topic, const String& property, const String& value) const
{
    MqttSettings.publishGeneric(topic, "{\"properties\": {\"" + property +  "\": " + value + "} }", false, 0);
}

template<typename... Arg>
void Provider::publishProperties(const String& topic, Arg&&... args) const
{
    static_assert((sizeof...(args) % 2) == 0);

    String out = "{\"properties\": {";
    bool even = true;
    bool first = true;
    for (const String d : std::initializer_list<String>({args...}))
    {
        if (even) {
            if (!first) {
                out += ", ";
            }
            out += "\"" + d + "\": ";
        } else {
            out += d;
        }
        even  = !even;
        first = false;
    }
    out += "} }";
    MqttSettings.publishGeneric(topic, out, false, 0);
}

void Provider::timesync()
{
    time_t now;
    if (!_topicTimesyncReply.isEmpty() && Utils::getEpoch(&now)) {
        MqttSettings.publishGeneric(_topicTimesyncReply, "{\"zoneOffset\": \"+00:00\", \"messageId\": " + String(++_messageCounter) + ", \"timestamp\": " + String(now) + "}", false, 0);
        DTU_LOGD("Timesync Reply");
    }
}

void Provider::setChargeThroughState(const ChargeThroughState value, const bool publish /* = true */)
{
    if (_stats->_charge_through_state.has_value() && value == *_stats->_charge_through_state) {
        return;
    }

    _stats->_charge_through_state = value;
    DTU_LOGD("Setting charge-through mode to '%s'!", Stats::chargeThroughStateToString(value));
    if (publish) {
        publishPersistentSettings(ZENDURE_PERSISTENT_SETTINGS_CHARGE_THROUGH, String(Stats::chargeThroughStateToString(value)));
    }

    // re-run suncalc to force updates in schedule mode
    rescheduleSunCalc();
}

void Provider::onMqttMessagePersistentSettings(espMqttClientTypes::MessageProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len)
{
    String t(topic);
    String string(reinterpret_cast<const char*>(payload), len);
    auto integer = static_cast<uint64_t>(string.toInt());

    DTU_LOGD("Received Persistent Settings %s = %s [aka %" PRId64 "]", topic, string.substring(0, 32).c_str(), integer);

    if (t.endsWith(ZENDURE_PERSISTENT_SETTINGS_LAST_FULL) && integer) {
        _stats->_last_full_timestamp = integer;
        return;
    }
    if (t.endsWith(ZENDURE_PERSISTENT_SETTINGS_LAST_EMPTY) && integer) {
        _stats->_last_empty_timestamp = integer;
        return;
    }
    if (t.endsWith(ZENDURE_PERSISTENT_SETTINGS_CHARGE_THROUGH)) {
        // only accept state from MQTT if not set during init
        if (_stats->_charge_through_state.has_value()) { return; }

        auto mode = Stats::chargeThroughStateFromString(string);
        if (mode.has_value()) {
            // interpret recent values if decodeable
            setChargeThroughState(*mode, false);
        } else {
            // otherwise, interpret legacy values that might be stored on broker
            setChargeThroughState(integer > 0 ? ChargeThroughState::Hard : ChargeThroughState::Idle, false);
        }

        return;
    }
}

void Provider::onMqttMessageTimesync(espMqttClientTypes::MessageProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len)
{
    timesync();
}

void Provider::onMqttMessageReport(espMqttClientTypes::MessageProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len)
{
    auto ms = millis();

    std::string const src = std::string(reinterpret_cast<const char*>(payload), len);
    std::string logValue = src.substr(0, 64);
    if (src.length() > logValue.length()) { logValue += "..."; }

    JsonDocument json;

    const DeserializationError error = deserializeJson(json, src);
    if (error) {
        DTU_LOGE("cannot parse payload '%s' as JSON", logValue.c_str());
        return;
    }

    if (json.overflowed()) {
        DTU_LOGE("payload too large to process as JSON");
        return;
    }

    auto obj = json.as<JsonObjectConst>();

    // validate input data
    // messageId has to be set to "123"
    // deviceId has to be set to the configured deviceId
    if (!json["messageId"].as<String>().equals("123")) {
        DTU_LOGE("Invalid or missing 'messageId' in '%s'", logValue.c_str());
        return;
    }
    if (!json["deviceId"].as<String>().equals(Configuration.get().Battery.Zendure.DeviceId)) {
        DTU_LOGE("Invalid or missing 'deviceId' in '%s'", logValue.c_str());
        return;
    }

    auto props = Utils::getJsonElement<JsonObjectConst>(obj, ZENDURE_REPORT_PROPERTIES, 1);
    if (props.has_value()) {
        auto sw_version = Utils::getJsonElement<uint32_t>(*props, ZENDURE_REPORT_MASTER_FW_VERSION);
        if (sw_version.has_value()) {
            _stats->setFwVersion(std::move(parseVersion(*sw_version)));
        }

        auto hw_version = Utils::getJsonElement<uint32_t>(*props, ZENDURE_REPORT_MASTER_HW_VERSION);
        if (hw_version.has_value()) {
            _stats->setHwVersion(std::move(parseVersion(*hw_version)));
        }

        auto soc_max = Utils::getJsonElement<float>(*props, ZENDURE_REPORT_MAX_SOC);
        if (soc_max.has_value()) {
            _stats->setSocMax(*soc_max / 10);
        }

        auto soc_min = Utils::getJsonElement<float>(*props, ZENDURE_REPORT_MIN_SOC);
        if (soc_min.has_value()) {
            _stats->setSocMin(*soc_min / 10);
        }

        auto input_limit = Utils::getJsonElement<uint16_t>(*props, ZENDURE_REPORT_INPUT_LIMIT);
        if (input_limit.has_value()) {
            _stats->_input_limit = *input_limit;
        }

        auto output_limit = Utils::getJsonElement<uint16_t>(*props, ZENDURE_REPORT_OUTPUT_LIMIT);
        if (output_limit.has_value()) {
            _stats->setOutputLimit(*output_limit);
        }

        auto inverse_max = Utils::getJsonElement<uint16_t>(*props, ZENDURE_REPORT_INVERSE_MAX_POWER);
        if (inverse_max.has_value()) {
            _stats->_inverse_max = *inverse_max;
        }

        auto state = Utils::getJsonElement<uint8_t>(*props, ZENDURE_REPORT_BATTERY_STATE);
        if (state.has_value() && *state <= 2) {
            _stats->_state = static_cast<State>(*state);
        }

        auto heat_state = Utils::getJsonElement<uint8_t>(*props, ZENDURE_REPORT_HEAT_STATE);
        if (heat_state.has_value()) {
            _stats->_heat_state = static_cast<bool>(*heat_state);
        }

        auto auto_shutdown = Utils::getJsonElement<uint8_t>(*props, ZENDURE_REPORT_AUTO_SHUTDOWN);
        if (auto_shutdown.has_value()) {
            _stats->_auto_shutdown = static_cast<bool>(*auto_shutdown);
        }

        auto auto_recover = Utils::getJsonElement<uint8_t>(*props, ZENDURE_REPORT_AUTO_RECOVER);
        if (auto_recover.has_value()) {
            _stats->setAutoRecover(*auto_recover);
        }

        auto buzzer = Utils::getJsonElement<uint8_t>(*props, ZENDURE_REPORT_BUZZER_SWITCH);
        if (buzzer.has_value()) {
            _stats->_buzzer = static_cast<bool>(*buzzer);
        }

        auto output_power = Utils::getJsonElement<uint16_t>(*props, ZENDURE_REPORT_OUTPUT_POWER);
        if (output_power.has_value()) {
            _stats->setOutputPower(*output_power);
        }

        auto discharge_power = Utils::getJsonElement<uint16_t>(*props, ZENDURE_REPORT_DISCHARGE_POWER);
        if (discharge_power.has_value()) {
            _stats->setDischargePower(*discharge_power);
        }

        auto charge_power = Utils::getJsonElement<uint16_t>(*props, ZENDURE_REPORT_CHARGE_POWER);
        if (charge_power.has_value()) {
            _stats->setChargePower(*charge_power);
        }

        auto solar_power_1 = Utils::getJsonElement<uint16_t>(*props, ZENDURE_REPORT_SOLAR_POWER_MPPT_1);
        if (solar_power_1.has_value()) {
            _stats->setSolarPower1(*solar_power_1);
        }

        auto solar_power_2 = Utils::getJsonElement<uint16_t>(*props, ZENDURE_REPORT_SOLAR_POWER_MPPT_2);
        if (solar_power_2.has_value()) {
            _stats->setSolarPower2(*solar_power_2);
        }

        auto bypass_mode = Utils::getJsonElement<uint8_t>(*props, ZENDURE_REPORT_BYPASS_MODE);
        if (bypass_mode.has_value() && *bypass_mode <= 2) {
            _stats->_bypass_mode = static_cast<BatteryZendureConfig::BypassMode_t>(*bypass_mode);
        }

        auto bypass_state = Utils::getJsonElement<uint8_t>(*props, ZENDURE_REPORT_BYPASS_STATE);
        if (bypass_state.has_value()) {
            _stats->_bypass_state = static_cast<bool>(*bypass_state);
        }

        _stats->_lastUpdate = ms;
    }

    // stop processing here, if no pack data found in message
    auto packData = Utils::getJsonElement<JsonArrayConst>(obj, ZENDURE_REPORT_PACK_DATA, 2);
    if (!packData.has_value() || _stats->_num_batteries == 0) {
        return;
    }

    // get serial number related to index only if all packs given in message
    if ((*packData).size() == _stats->_num_batteries) {
        for (size_t i = 0 ; i < _stats->_num_batteries ; i++) {
            auto serial = Utils::getJsonElement<String>((*packData)[i], ZENDURE_REPORT_PACK_SERIAL);
            if (!serial.has_value()) {
                DTU_LOGW("Missing serial of battery pack in '%s'", logValue.c_str());
                continue;
            }
            if (_stats->addPackData(i+1, *serial) == nullptr) {
                DTU_LOGW("Invalid or unknown serial '%s' in '%s'", (*serial).c_str(), logValue.c_str());
            }
        }
    }

    // check if our array has got inconsistant
    if (_stats->_packData.size() > _stats->_num_batteries) {
        DTU_LOGD("Detected inconsitency of pack data - resetting internal data buffer!");
        _stats->_packData.clear();
        return;
    }

    // get additional data only if all packs were identified
    if (_stats->_packData.size() != _stats->_num_batteries) {
        return;
    }


    for (auto packDataJson : *packData) {
        auto serial = Utils::getJsonElement<String>(packDataJson, ZENDURE_REPORT_PACK_SERIAL);
        auto state = Utils::getJsonElement<uint8_t>(packDataJson, ZENDURE_REPORT_PACK_STATE);
        auto version = Utils::getJsonElement<uint32_t>(packDataJson, ZENDURE_REPORT_PACK_FW_VERSION);
        auto soh = Utils::getJsonElement<uint16_t>(packDataJson, ZENDURE_REPORT_PACK_HEALTH);
        auto voltage = Utils::getJsonElement<uint16_t>(packDataJson, ZENDURE_REPORT_PACK_TOTAL_VOLTAGE);

        // do not waste processing time if nothing to do
        if (!serial.has_value() || !(state.has_value() || version.has_value() || soh.has_value() || voltage.has_value())) {
            continue;
        }

        // find pack data related to serial number
        for (auto& entry : _stats->_packData) {
            auto pack = entry.second;
            if (pack->_serial != serial) {
                continue;
            }
            if (state.has_value()) {
                pack->_state = static_cast<State>(*state);
            }

            if (version.has_value()) {
                pack->setFwVersion(std::move(parseVersion(*version)));
            }

            if (soh.has_value()) {
                pack->setSoH(static_cast<float>(*soh) / 10.0);
            }

            // Fallback to voltage reported by the FIRST pack if we are unable to use values from loggin messages.
            // This is only precise, if there is exactly ONE pack - when there are more packs, using only the first
            // is not sufficient and voltage will be completely unavailable to prevent erroneous reporting.
            if (voltage.has_value() && !_full_log_supported && entry.first == 1 && _stats->_num_batteries == 1) {
                _stats->setVoltage(static_cast<float>(*voltage) / 100.0, ms);
            }

            pack->_lastUpdate = ms;

            // found the pack we searched for, so terminate loop here
            break;
        }
    }
}

void Provider::onMqttMessageLog(espMqttClientTypes::MessageProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len)
{
    auto ms = millis();

    DTU_LOGD("Logging Frame received!");

    std::string const src = std::string(reinterpret_cast<const char*>(payload), len);
    std::string logValue = src.substr(0, 64);
    if (src.length() > logValue.length()) { logValue += "..."; }

    JsonDocument json;

    const DeserializationError error = deserializeJson(json, src);
    if (error) {
        DTU_LOGE("cannot parse payload '%s' as JSON", logValue.c_str());
        return;
    }

    if (json.overflowed()) {
        DTU_LOGE("payload too large to process as JSON");
        return;
    }

    auto obj = json.as<JsonObjectConst>();

    // validate input data
    // deviceId has to be set to the configured deviceId
    // logType has to be set to "2"
    if (!json["deviceId"].as<String>().equals(Configuration.get().Battery.Zendure.DeviceId)) {
        DTU_LOGE("Invalid or missing 'deviceId' in '%s'", logValue.c_str());
        return;
    }
    if (!json["logType"].as<String>().equals("2")) {
        DTU_LOGE("Invalid or missing 'v' in '%s'", logValue.c_str());
        return;
    }

    auto data = Utils::getJsonElement<JsonObjectConst>(obj, ZENDURE_LOG_ROOT, 2);
    if (!data.has_value()) {
        DTU_LOGE("Unable to find 'log' in '%s'", logValue.c_str());
        return;
    }

    _stats->setSerial(Utils::getJsonElement<String>(*data, ZENDURE_LOG_SERIAL));

    auto params = Utils::getJsonElement<JsonArrayConst>(*data, ZENDURE_LOG_PARAMS, 1);
    if (!params.has_value()) {
        DTU_LOGE("Unable to find 'params' in '%s'", logValue.c_str());
        return;
    }

    auto v = *params;

    uint16_t soc = 0;
    uint16_t voltage = 0;
    int16_t power = 0;
    int16_t current = 0;
    uint16_t capacity = 0;
    uint16_t capacity_avail = 0;

    uint8_t num = v[ZENDURE_LOG_OFFSET_PACKNUM].as<uint8_t>();
    if (num > 0 && num <= ZENDURE_MAX_PACKS) {
        uint32_t cellMin = UINT32_MAX;
        uint32_t cellMax = 0;
        uint32_t cellAvg = 0;
        uint32_t cellDelta = 0;
        int32_t cellTemp = 0;

        for (size_t i = 1 ; i <= num ; i++) {
            uint16_t pvol = v[ZENDURE_LOG_OFFSET_PACK_VOLTAGE(i)].as<uint16_t>() * 10;
            int16_t  pcur = v[ZENDURE_LOG_OFFSET_PACK_CURRENT(i)].as<int16_t>();
            uint16_t psoc = v[ZENDURE_LOG_OFFSET_PACK_SOC(i)].as<uint16_t>();

            int32_t  ctmp = v[ZENDURE_LOG_OFFSET_PACK_TEMPERATURE(i)].as<int32_t>();
            uint32_t cmin = v[ZENDURE_LOG_OFFSET_PACK_CELL_MIN(i)].as<uint32_t>() * 10;
            uint32_t cmax = v[ZENDURE_LOG_OFFSET_PACK_CELL_MAX(i)].as<uint32_t>() * 10;
            uint32_t cdel = cmax - cmin;

            auto pack = _stats->getPackData(i);
            if (pack != nullptr) {
                auto cavg = pvol / pack->getCellCount();

                pack->_cell_voltage_min = static_cast<uint16_t>(cmin);
                pack->_cell_voltage_max = static_cast<uint16_t>(cmax);
                pack->_cell_voltage_avg = static_cast<uint16_t>(cavg);
                pack->_cell_voltage_spread = static_cast<uint16_t>(cdel);
                pack->_cell_temperature_max = static_cast<int16_t>(ctmp);
                pack->_current = static_cast<float>(pcur) / 10.0;
                pack->_voltage_total = static_cast<float>(pvol) / 1000.0;
                pack->_soc_level = static_cast<float>(psoc) / 10.0;
                pack->_power = static_cast<int16_t>(pack->_current * pack->_voltage_total);
                pack->_lastUpdate = ms;

                capacity_avail += pack->_capacity_avail;
                capacity += pack->_capacity;
                cellAvg += cavg;
                power += pack->_power;
            }

            cellMin = min(cmin, cellMin);
            cellMax = max(cmax, cellMax);
            cellDelta = max(cdel, cellDelta);
            cellTemp = max(ctmp, cellTemp);

            soc += psoc;
            voltage += pvol;
            current += pcur;
        }

        _stats->_cellMinMilliVolt = static_cast<uint16_t>(cellMin);
        _stats->_cellMaxMilliVolt = static_cast<uint16_t>(cellMax);
        _stats->_cellAvgMilliVolt = static_cast<uint16_t>(cellAvg) / num;
        _stats->_cellDeltaMilliVolt = static_cast<uint16_t>(cellDelta);
        _stats->_cellTemperature = static_cast<int16_t>(cellTemp);
    }

    setSoC(static_cast<float>(soc) / 10.0 / num, ms);

    _stats->_num_batteries = num;
    _stats->_capacity = capacity;
    _stats->_capacity_avail = capacity_avail;

    _stats->setCurrent(static_cast<float>(current) / 10.0, 1, ms);

    // some devices have different log structure - only process for devices explicitly enabled!
    if (_full_log_supported) {
        _stats->setVoltage(v[ZENDURE_LOG_OFFSET_VOLTAGE].as<float>() / 10.0, ms);

        _stats->setAutoRecover(v[ZENDURE_LOG_OFFSET_AUTO_RECOVER].as<uint8_t>());
        _stats->setSocMin(v[ZENDURE_LOG_OFFSET_MIN_SOC].as<float>());

        _stats->setOutputLimit(static_cast<uint16_t>(v[ZENDURE_LOG_OFFSET_OUTPUT_POWER_LIMIT].as<uint32_t>() / 100));
        _stats->setOutputPower(v[ZENDURE_LOG_OFFSET_OUTPUT_POWER].as<uint16_t>());
        _stats->setChargePower(v[ZENDURE_LOG_OFFSET_CHARGE_POWER].as<uint16_t>());
        _stats->setDischargePower(v[ZENDURE_LOG_OFFSET_DISCHARGE_POWER].as<uint16_t>());
        _stats->setSolarPower1(v[ZENDURE_LOG_OFFSET_SOLAR_POWER_MPPT_1].as<uint16_t>());
        _stats->setSolarPower2(v[ZENDURE_LOG_OFFSET_SOLAR_POWER_MPPT_2].as<uint16_t>());
    }

    _stats->_lastUpdate = ms;
    calculateEfficiency();
}

String Provider::parseVersion(uint32_t version)
{
    if (version == 0) {
        return String();
    }

    uint8_t major = (version >> 12) & 0xF;
    uint8_t minor = (version >> 8) & 0xF;
    uint8_t bugfix = version & 0xFF;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%" PRIu8 ".%" PRIu8 ".%" PRIu8, major, minor, bugfix);
    return String(buffer);
}

void Provider::calculateEfficiency()
{
    float in = static_cast<float>(_stats->_input_power);
    float out = static_cast<float>(_stats->_output_power);
    float efficiency = 0.0;

    in += static_cast<float>(_stats->_discharge_power);
    out += static_cast<float>(_stats->_charge_power);

    if (in <= 0.0) {
        _stats->_efficiency.reset();
        return;
    }

    efficiency = out / in;

    if (efficiency > 1.0 || efficiency < 0.0) {
        _stats->_efficiency.reset();
        return;
    }

    _stats->_efficiency = efficiency * 100;
}

void Provider::setSoC(const float soc, const uint32_t timestamp /* = 0 */, const uint8_t precision /* = 2 */)
{
    time_t now;
    auto const& config = Configuration.get();
    auto const chargeThroughState = _stats->_charge_through_state.value_or(ChargeThroughState::Disabled);

    if (Utils::getEpoch(&now)) {
        if (soc >= 100.0) {
            _stats->_last_full_timestamp = now;
            publishPersistentSettings(ZENDURE_PERSISTENT_SETTINGS_LAST_FULL, String(now));

            if (chargeThroughState == ChargeThroughState::Soft || chargeThroughState == ChargeThroughState::Hard) {
                setChargeThroughState(ChargeThroughState::Keep);
            }
        }
        if (soc < static_cast<float>(config.Battery.Zendure.ChargeThroughResetLevel) && chargeThroughState == ChargeThroughState::Keep) {
            setChargeThroughState(ChargeThroughState::Idle);
        }
        if (soc <= 0.0) {
            _stats->_last_empty_timestamp = now;
            publishPersistentSettings(ZENDURE_PERSISTENT_SETTINGS_LAST_EMPTY, String(now));
        }
    }

    _stats->setSoC(soc, precision, timestamp ? timestamp : millis());
}

void Provider::publishPersistentSettings(const char* subtopic, const String& payload)
{
    if (!_topicPersistentSettings.isEmpty())
    {
        DTU_LOGD("Writing Persistent Settings %s = %s\r\n",
                String(_topicPersistentSettings + subtopic).c_str(),
                payload.substring(0, 32).c_str());
        MqttSettings.publishGeneric(_topicPersistentSettings + subtopic, payload, true);
    }
}


} // namespace Batteries::Zendure
