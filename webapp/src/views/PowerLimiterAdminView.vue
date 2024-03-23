<template>
    <BasePage :title="$t('powerlimiteradmin.PowerLimiterSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <BootstrapAlert v-model="configAlert" variant="warning">
            {{ $t('powerlimiteradmin.ConfigAlertMessage') }}
        </BootstrapAlert>

        <CardElement :text="$t('powerlimiteradmin.ConfigHints')" textVariant="text-bg-primary" v-if="getConfigHints().length">
            <div class="row">
                <div class="col-sm-12">
                    {{ $t('powerlimiteradmin.ConfigHintsIntro') }}
                    <ul class="mb-0">
                        <li v-for="(hint, idx) in getConfigHints()" :key="idx">
                            <b v-if="hint.severity === 'requirement'">{{ $t('powerlimiteradmin.ConfigHintRequirement') }}:</b>
                            <b v-if="hint.severity === 'optional'">{{ $t('powerlimiteradmin.ConfigHintOptional') }}:</b>
                            {{ $t('powerlimiteradmin.ConfigHint' + hint.subject) }}
                        </li>
                    </ul>
                </div>
            </div>
        </CardElement>

        <form @submit="savePowerLimiterConfig" v-if="!configAlert">
            <CardElement :text="$t('powerlimiteradmin.General')" textVariant="text-bg-primary" add-space>
                <InputElement :label="$t('powerlimiteradmin.Enable')"
                              v-model="powerLimiterConfigList.enabled"
                              type="checkbox" wide/>

                <InputElement v-show="isEnabled()"
                              :label="$t('powerlimiteradmin.VerboseLogging')"
                              v-model="powerLimiterConfigList.verbose_logging"
                              type="checkbox" wide/>

                <InputElement v-show="isEnabled()"
                              :label="$t('powerlimiteradmin.TargetPowerConsumption')"
                              :tooltip="$t('powerlimiteradmin.TargetPowerConsumptionHint')"
                              v-model="powerLimiterConfigList.target_power_consumption"
                              postfix="W"
                              type="number" wide/>

                <InputElement v-show="isEnabled()"
                              :label="$t('powerlimiteradmin.TargetPowerConsumptionHysteresis')"
                              :tooltip="$t('powerlimiteradmin.TargetPowerConsumptionHysteresisHint')"
                              v-model="powerLimiterConfigList.target_power_consumption_hysteresis"
                              postfix="W"
                              type="number" wide/>
            </CardElement>

            <CardElement :text="$t('powerlimiteradmin.InverterSettings')" textVariant="text-bg-primary" add-space v-if="isEnabled()">
                <div class="row mb-3">
                    <label for="inverter_serial" class="col-sm-4 col-form-label">
                        {{ $t('powerlimiteradmin.Inverter') }}
                    </label>
                    <div class="col-sm-8">
                        <select id="inverter_serial" class="form-select" v-model="powerLimiterConfigList.inverter_serial" required>
                            <option value="" disabled hidden selected>{{ $t('powerlimiteradmin.SelectInverter') }}</option>
                            <option v-for="(inv, serial) in powerLimiterMetaData.inverters" :key="serial" :value="serial">
                                {{ inv.name }} ({{ inv.type }})
                            </option>
                        </select>
                    </div>
                </div>

                <InputElement :label="$t('powerlimiteradmin.InverterIsSolarPowered')"
                              v-model="powerLimiterConfigList.is_inverter_solar_powered"
                              type="checkbox" wide/>

                <div class="row mb-3" v-if="needsChannelSelection()">
                    <label for="inverter_channel" class="col-sm-4 col-form-label">
                        {{ $t('powerlimiteradmin.InverterChannelId') }}
                    </label>
                    <div class="col-sm-8">
                        <select id="inverter_channel" class="form-select" v-model="powerLimiterConfigList.inverter_channel_id">
                            <option v-for="channel in range(powerLimiterMetaData.inverters[powerLimiterConfigList.inverter_serial].channels)" :key="channel" :value="channel">
                                {{ channel + 1 }}
                            </option>
                        </select>
                    </div>
                </div>

                <InputElement :label="$t('powerlimiteradmin.LowerPowerLimit')"
                              v-model="powerLimiterConfigList.lower_power_limit"
                              placeholder="50" min="10" postfix="W"
                              type="number" wide/>

                <InputElement :label="$t('powerlimiteradmin.UpperPowerLimit')"
                              v-model="powerLimiterConfigList.upper_power_limit"
                              placeholder="800" min="20" postfix="W"
                              type="number" wide/>

                <InputElement :label="$t('powerlimiteradmin.InverterIsBehindPowerMeter')"
                              v-model="powerLimiterConfigList.is_inverter_behind_powermeter"
                              type="checkbox" wide/>

                <div class="row mb-3" v-if="!powerLimiterConfigList.is_inverter_solar_powered">
                    <label for="inverter_restart" class="col-sm-4 col-form-label">
                        {{ $t('powerlimiteradmin.InverterRestartHour') }}
                        <BIconInfoCircle v-tooltip :title="$t('powerlimiteradmin.InverterRestartHint')" />
                    </label>
                    <div class="col-sm-8">
                        <select id="inverter_restart" class="form-select" v-model="powerLimiterConfigList.inverter_restart_hour">
                            <option value="-1">
                                {{ $t('powerlimiteradmin.InverterRestartDisabled') }}
                            </option>
                            <option v-for="hour in range(24)" :key="hour" :value="hour">
                                {{ (hour > 9) ? hour : "0"+hour }}:00
                            </option>
                        </select>
                    </div>
                </div>
            </CardElement>

            <CardElement :text="$t('powerlimiteradmin.SolarPassthrough')" textVariant="text-bg-primary" add-space v-if="canUseSolarPassthrough()">
                <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.SolarpassthroughInfo')"></div>

                <InputElement :label="$t('powerlimiteradmin.EnableSolarPassthrough')"
                              v-model="powerLimiterConfigList.solar_passthrough_enabled"
                              type="checkbox" wide/>

                <div v-if="powerLimiterConfigList.solar_passthrough_enabled">
                    <InputElement :label="$t('powerlimiteradmin.BatteryDischargeAtNight')"
                                  v-model="powerLimiterConfigList.battery_always_use_at_night"
                                  type="checkbox" wide/>

                    <InputElement :label="$t('powerlimiteradmin.SolarPassthroughLosses')"
                                  v-model="powerLimiterConfigList.solar_passthrough_losses"
                                  placeholder="3" min="0" max="10" postfix="%"
                                  type="number" wide/>

                    <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.SolarPassthroughLossesInfo')"></div>
                </div>
            </CardElement>

            <CardElement :text="$t('powerlimiteradmin.SocThresholds')" textVariant="text-bg-primary" add-space v-if="canUseSoCThresholds()">
                <InputElement
                    :label="$t('powerlimiteradmin.IgnoreSoc')"
                    v-model="powerLimiterConfigList.ignore_soc"
                    type="checkbox" wide/>

                <div v-if="!powerLimiterConfigList.ignore_soc">
                    <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.BatterySocInfo')"></div>

                    <InputElement :label="$t('powerlimiteradmin.StartThreshold')"
                                  v-model="powerLimiterConfigList.battery_soc_start_threshold"
                                  placeholder="80" min="0" max="100" postfix="%"
                                  type="number" wide/>

                    <InputElement :label="$t('powerlimiteradmin.StopThreshold')"
                                  v-model="powerLimiterConfigList.battery_soc_stop_threshold"
                                  placeholder="20" min="0" max="100" postfix="%"
                                  type="number" wide/>

                    <InputElement :label="$t('powerlimiteradmin.FullSolarPassthroughStartThreshold')"
                                  :tooltip="$t('powerlimiteradmin.FullSolarPassthroughStartThresholdHint')"
                                  v-model="powerLimiterConfigList.full_solar_passthrough_soc"
                                  v-if="isSolarPassthroughEnabled()"
                                  placeholder="80" min="0" max="100" postfix="%"
                                  type="number" wide/>
                </div>
            </CardElement>

            <CardElement :text="$t('powerlimiteradmin.VoltageThresholds')" textVariant="text-bg-primary" add-space v-if="canUseVoltageThresholds()">
                <InputElement :label="$t('powerlimiteradmin.StartThreshold')"
                              v-model="powerLimiterConfigList.voltage_start_threshold"
                              placeholder="50" min="16" max="66" postfix="V"
                              type="number" step="0.01" wide/>

                <InputElement :label="$t('powerlimiteradmin.StopThreshold')"
                              v-model="powerLimiterConfigList.voltage_stop_threshold"
                              placeholder="49" min="16" max="66" postfix="V"
                              type="number" step="0.01" wide/>

                <div v-if="isSolarPassthroughEnabled()">
                    <InputElement :label="$t('powerlimiteradmin.FullSolarPassthroughStartThreshold')"
                                  :tooltip="$t('powerlimiteradmin.FullSolarPassthroughStartThresholdHint')"
                                  v-model="powerLimiterConfigList.full_solar_passthrough_start_voltage"
                                  placeholder="49" min="16" max="66" postfix="V"
                                  type="number" step="0.01" wide/>

                    <InputElement :label="$t('powerlimiteradmin.VoltageSolarPassthroughStopThreshold')"
                                  v-model="powerLimiterConfigList.full_solar_passthrough_stop_voltage"
                                  placeholder="49" min="16" max="66" postfix="V"
                                  type="number" step="0.01" wide/>
                </div>

                <InputElement :label="$t('powerlimiteradmin.VoltageLoadCorrectionFactor')"
                              v-model="powerLimiterConfigList.voltage_load_correction_factor"
                              placeholder="0.0001" postfix="1/A"
                              type="number" step="0.0001" wide/>

                <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.VoltageLoadCorrectionInfo')"></div>
            </CardElement>

            <FormFooter @reload="getAllData"/>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import { BIconInfoCircle } from 'bootstrap-icons-vue';
import type { PowerLimiterConfig, PowerLimiterMetaData } from "@/types/PowerLimiterConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
        InputElement,
        BIconInfoCircle,
    },
    data() {
        return {
            dataLoading: true,
            powerLimiterConfigList: {} as PowerLimiterConfig,
            powerLimiterMetaData: {} as PowerLimiterMetaData,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            configAlert: false,
        };
    },
    created() {
        this.getAllData();
    },
    watch: {
        'powerLimiterConfigList.inverter_serial'(newVal) {
            var cfg = this.powerLimiterConfigList;
            var meta = this.powerLimiterMetaData;

            if (newVal === "") { return; } // do not try to convert the placeholder value

            if (meta.inverters[newVal] !== undefined) { return; }

            for (const [serial, inverter] of Object.entries(meta.inverters)) {
                // cfg.inverter_serial might be too large to parse as a 32 bit
                // int, so we make sure to only try to parse two characters. if
                // cfg.inverter_serial is indeed an old position based index,
                // it is only one character.
                if (inverter.pos == Number(cfg.inverter_serial.substr(0, 2))) {
                    // inverter_serial uses the old position-based
                    // value to identify the inverter. convert to serial.
                    cfg.inverter_serial = serial;
                    return;
                }
            }

            // previously selected inverter was deleted. marks serial as
            // invalid, selects placeholder option.
            cfg.inverter_serial = '';
        }
    },
    methods: {
        getConfigHints() {
            var cfg = this.powerLimiterConfigList;
            var meta = this.powerLimiterMetaData;
            var hints = [];

            if (meta.power_meter_enabled !== true) {
                hints.push({severity: "requirement", subject: "PowerMeterDisabled"});
                this.configAlert = true;
            }

            if (typeof meta.inverters === "undefined" || Object.keys(meta.inverters).length == 0) {
                hints.push({severity: "requirement", subject: "NoInverter"});
                this.configAlert = true;
            }
            else {
                var inv = meta.inverters[cfg.inverter_serial];
                if (inv !== undefined && !(inv.poll_enable && inv.command_enable && inv.poll_enable_night && inv.command_enable_night)) {
                    hints.push({severity: "requirement", subject: "InverterCommunication"});
                }
            }

            if (!cfg.is_inverter_solar_powered) {
                if (!meta.charge_controller_enabled) {
                    hints.push({severity: "optional", subject: "NoChargeController"});
                }

                if (!meta.battery_enabled) {
                    hints.push({severity: "optional", subject: "NoBatteryInterface"});
                }
            }

            return hints;
        },
        isEnabled() {
            return this.powerLimiterConfigList.enabled;
        },
        canUseSolarPassthrough() {
            var cfg = this.powerLimiterConfigList;
            var meta = this.powerLimiterMetaData;
            var canUse = this.isEnabled() && meta.charge_controller_enabled && !cfg.is_inverter_solar_powered;
            if (!canUse) { cfg.solar_passthrough_enabled = false; }
            return canUse;
        },
        canUseSoCThresholds() {
            var cfg = this.powerLimiterConfigList;
            var meta = this.powerLimiterMetaData;
            return this.isEnabled() && meta.battery_enabled && !cfg.is_inverter_solar_powered;
        },
        canUseVoltageThresholds() {
            var cfg = this.powerLimiterConfigList;
            return this.isEnabled() && !cfg.is_inverter_solar_powered;
        },
        isSolarPassthroughEnabled() {
            return this.powerLimiterConfigList.solar_passthrough_enabled;
        },
        range(end: number) {
            return Array.from(Array(end).keys());
        },
        needsChannelSelection() {
            var cfg = this.powerLimiterConfigList;
            var meta = this.powerLimiterMetaData;

            var reset = function() {
                cfg.inverter_channel_id = 0;
                return false;
            };

            if (cfg.inverter_serial === '') { return reset(); }

            if (cfg.is_inverter_solar_powered) { return reset(); }

            var inverter = meta.inverters[cfg.inverter_serial];
            if (inverter === undefined) { return reset(); }

            if (cfg.inverter_channel_id >= inverter.channels) {
                reset();
            }

            return inverter.channels > 1;
        },
        getAllData() {
            this.dataLoading = true;
            fetch("/api/powerlimiter/metadata", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.powerLimiterMetaData = data;
                    fetch("/api/powerlimiter/config", { headers: authHeader() })
                        .then((response) => handleResponse(response, this.$emitter, this.$router))
                        .then((data) => {
                            this.powerLimiterConfigList = data;
                            this.dataLoading = false;
                        });
                });
        },
        savePowerLimiterConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.powerLimiterConfigList));

            fetch("/api/powerlimiter/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.alertMessage = response.message;
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>
