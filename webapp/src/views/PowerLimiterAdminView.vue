<template>
    <BasePage :title="'Dynamic Power limiter Settings'" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="savePowerLimiterConfig">
            <CardElement :text="$t('powerlimiteradmin.General')" textVariant="text-bg-primary">
                <InputElement :label="$t('powerlimiteradmin.Enable')"
                                v-model="powerLimiterConfigList.enabled"
                                type="checkbox" wide/>

                <InputElement v-show="powerLimiterConfigList.enabled"
                                :label="$t('powerlimiteradmin.VerboseLogging')"
                                v-model="powerLimiterConfigList.verbose_logging"
                                type="checkbox" wide/>

                <InputElement v-show="powerLimiterConfigList.enabled"
                                :label="$t('powerlimiteradmin.EnableSolarPassthrough')"
                                v-model="powerLimiterConfigList.solar_passthrough_enabled"
                                type="checkbox" wide/>
                
                <div class="row mb-3" v-show="powerLimiterConfigList.enabled && powerLimiterConfigList.solar_passthrough_enabled">
                    <label for="inputTimezone" class="col-sm-2 col-form-label">
                        {{ $t('powerlimiteradmin.BatteryDrainStrategy') }}:
                    </label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="powerLimiterConfigList.battery_drain_strategy">
                            <option v-for="batteryDrainStrategy in batteryDrainStrategyList" :key="batteryDrainStrategy.key" :value="batteryDrainStrategy.key">
                                {{ $t(batteryDrainStrategy.value) }}
                            </option>
                        </select>
                    </div>
                </div>

                <div class="alert alert-secondary" v-show="powerLimiterConfigList.enabled" role="alert" v-html="$t('powerlimiteradmin.SolarpassthroughInfo')"></div>

                <div class="row mb-3" v-show="powerLimiterConfigList.enabled && powerLimiterConfigList.solar_passthrough_enabled">
                    <label for="solarPassthroughLosses" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.SolarPassthroughLosses') }}</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" class="form-control" id="solarPassthroughLosses"
                                placeholder="3" v-model="powerLimiterConfigList.solar_passthrough_losses"
                                aria-describedby="solarPassthroughLossesDescription" min="0" max="10" required/>
                                <span class="input-group-text" id="solarPassthroughLossesDescription">%</span>
                        </div>
                    </div>
                </div>

                <div class="alert alert-secondary" role="alert" v-show="powerLimiterConfigList.enabled && powerLimiterConfigList.solar_passthrough_enabled" v-html="$t('powerlimiteradmin.SolarPassthroughLossesInfo')"></div>

                <div class="row mb-3" v-show="powerLimiterConfigList.enabled">
                    <label for="inputTimezone" class="col-sm-2 col-form-label">
                        {{ $t('powerlimiteradmin.InverterId') }}:
                        <BIconInfoCircle v-tooltip :title="$t('powerlimiteradmin.InverterIdHint')" />
                    </label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="powerLimiterConfigList.inverter_id">
                            <option v-for="inverter in inverterList" :key="inverter.key" :value="inverter.key">
                                {{ inverter.value }}
                            </option>
                        </select>
                    </div>
                </div>

                <div class="row mb-3" v-show="powerLimiterConfigList.enabled">
                    <label for="inputTimezone" class="col-sm-2 col-form-label">
                        {{ $t('powerlimiteradmin.InverterChannelId') }}:
                        <BIconInfoCircle v-tooltip :title="$t('powerlimiteradmin.InverterChannelIdHint')" />
                    </label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="powerLimiterConfigList.inverter_channel_id">
                            <option v-for="inverterChannel in inverterChannelList" :key="inverterChannel.key" :value="inverterChannel.key">
                                {{ inverterChannel.value }}
                            </option>
                        </select>
                    </div>
                </div>

                <InputElement v-show="powerLimiterConfigList.enabled"
                    :label="$t('powerlimiteradmin.InverterIsSolarPowered')"
                    v-model="powerLimiterConfigList.is_inverter_solar_powered"
                    type="checkbox" wide/>

                <div class="row mb-3" v-show="powerLimiterConfigList.enabled">
                    <label for="targetPowerConsumption" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.TargetPowerConsumption') }}:
                        <BIconInfoCircle v-tooltip :title="$t('powerlimiteradmin.TargetPowerConsumptionHint')" />
                    </label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" class="form-control" id="targetPowerConsumption"
                                placeholder="75" v-model="powerLimiterConfigList.target_power_consumption"
                                aria-describedby="targetPowerConsumptionDescription" required/>
                                <span class="input-group-text" id="targetPowerConsumptionDescription">W</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3" v-show="powerLimiterConfigList.enabled">
                    <label for="targetPowerConsumptionHyteresis" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.TargetPowerConsumptionHysteresis') }}:
                        <BIconInfoCircle v-tooltip :title="$t('powerlimiteradmin.TargetPowerConsumptionHysteresisHint')" required/>
                    </label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" class="form-control" id="targetPowerConsumptionHysteresis"
                                placeholder="30" min="0" v-model="powerLimiterConfigList.target_power_consumption_hysteresis"
                                aria-describedby="targetPowerConsumptionHysteresisDescription" />
                                <span class="input-group-text" id="targetPowerConsumptionHysteresisDescription">W</span>
                        </div>
                    </div>
                </div>
                
                <div class="row mb-3" v-show="powerLimiterConfigList.enabled">
                    <label for="inputLowerPowerLimit" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.LowerPowerLimit') }}:</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" class="form-control" id="inputLowerPowerLimit"
                                placeholder="50" min="10" v-model="powerLimiterConfigList.lower_power_limit"
                                aria-describedby="lowerPowerLimitDescription" required/>
                                <span class="input-group-text" id="lowerPowerLimitDescription">W</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3" v-show="powerLimiterConfigList.enabled">
                    <label for="inputUpperPowerLimit" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.UpperPowerLimit') }}:</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" class="form-control" id="inputUpperPowerLimit"
                                placeholder="800" v-model="powerLimiterConfigList.upper_power_limit"
                                aria-describedby="upperPowerLimitDescription" required/>
                            <span class="input-group-text" id="upperPowerLimitDescription">W</span>
                        </div>
                    </div>
                </div>
            </CardElement>

            <CardElement :text="$t('powerlimiteradmin.PowerMeters')" textVariant="text-bg-primary" add-space
                         v-show="powerLimiterConfigList.enabled"
            >
                <InputElement 
                    :label="$t('powerlimiteradmin.InverterIsBehindPowerMeter')"
                    v-model="powerLimiterConfigList.is_inverter_behind_powermeter"
                    type="checkbox" wide/>
            </CardElement>

            <CardElement :text="$t('powerlimiteradmin.Battery')" textVariant="text-bg-primary" add-space
                         v-show="powerLimiterConfigList.enabled"
            >
                <InputElement
                    :label="$t('powerlimiteradmin.IgnoreSoc')"
                    v-model="powerLimiterConfigList.ignore_soc"
                    type="checkbox"/>

                <div class="row mb-3" v-show="!powerLimiterConfigList.ignore_soc">
                    <label for="batterySocStartThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.BatterySocStartThreshold') }}:</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" class="form-control" id="batterySocStartThreshold"
                                placeholder="80" v-model="powerLimiterConfigList.battery_soc_start_threshold"
                                aria-describedby="batterySocStartThresholdDescription" min="0" max="100" required/>
                                <span class="input-group-text" id="batterySocStartThresholdDescription">%</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3" v-show="!powerLimiterConfigList.ignore_soc">
                    <label for="batterySocStopThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.BatterySocStopThreshold') }}</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" class="form-control" id="batterySocStopThreshold"
                                placeholder="20" v-model="powerLimiterConfigList.battery_soc_stop_threshold"
                                aria-describedby="batterySocStopThresholdDescription" min="0" max="100" required/>
                                <span class="input-group-text" id="batterySocStopThresholdDescription">%</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3" v-show="powerLimiterConfigList.solar_passthrough_enabled && !powerLimiterConfigList.ignore_soc">
                    <label for="batterySocSolarPassthroughStartThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.BatterySocSolarPassthroughStartThreshold') }}
                      <BIconInfoCircle v-tooltip :title="$t('powerlimiteradmin.BatterySocSolarPassthroughStartThresholdHint')" />
                    </label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" class="form-control" id="batterySocSolarPassthroughStartThreshold"
                                placeholder="20" v-model="powerLimiterConfigList.full_solar_passthrough_soc"
                                aria-describedby="batterySocSolarPassthroughStartThresholdDescription" min="0" max="100" required/>
                                <span class="input-group-text" id="batterySocSolarPassthroughStartThresholdDescription">%</span>
                        </div>
                    </div>
                </div>

                <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.BatterySocInfo')" v-show="!powerLimiterConfigList.ignore_soc"></div>

                <div class="row mb-3">
                    <label for="inputVoltageStartThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.VoltageStartThreshold') }}:</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" step="0.01" class="form-control" id="inputVoltageStartThreshold"
                                placeholder="50" v-model="powerLimiterConfigList.voltage_start_threshold"
                                aria-describedby="voltageStartThresholdDescription" required/>
                                <span class="input-group-text" id="voltageStartThresholdDescription">V</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3">
                    <label for="inputVoltageStopThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.VoltageStopThreshold') }}:</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" step="0.01" class="form-control" id="inputVoltageStopThreshold"
                                placeholder="49" v-model="powerLimiterConfigList.voltage_stop_threshold"
                                aria-describedby="voltageStopThresholdDescription" required/>
                            <span class="input-group-text" id="voltageStopThresholdDescription">V</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3" v-show="powerLimiterConfigList.solar_passthrough_enabled">
                    <label for="inputVoltageSolarPassthroughStartThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.VoltageSolarPassthroughStartThreshold') }}:
                      <BIconInfoCircle v-tooltip :title="$t('powerlimiteradmin.VoltageSolarPassthroughStartThresholdHint')" />
                    </label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" step="0.01" class="form-control" id="inputVoltageSolarPassthroughStartThreshold"
                                placeholder="49" v-model="powerLimiterConfigList.full_solar_passthrough_start_voltage"
                                aria-describedby="voltageSolarPassthroughStartThresholdDescription" required/>
                            <span class="input-group-text" id="voltageSolarPassthroughStartThresholdDescription">V</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3"  v-show="powerLimiterConfigList.solar_passthrough_enabled">
                    <label for="inputVoltageSolarPassthroughStopThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.VoltageSolarPassthroughStopThreshold') }}:</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" step="0.01" class="form-control" id="inputVoltageSolarPassthroughStopThreshold"
                                placeholder="49" v-model="powerLimiterConfigList.full_solar_passthrough_stop_voltage"
                                aria-describedby="voltageSolarPassthroughStopThresholdDescription" required/>
                            <span class="input-group-text" id="voltageSolarPassthroughStopThresholdDescription">V</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3">
                    <label for="inputVoltageLoadCorrectionFactor" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.VoltageLoadCorrectionFactor') }}:</label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="number" step="0.0001" class="form-control" id="inputVoltageLoadCorrectionFactor"
                                placeholder="49" v-model="powerLimiterConfigList.voltage_load_correction_factor"
                                aria-describedby="voltageLoadCorrectionFactorDescription" required/>
                            <span class="input-group-text" id="voltageLoadCorrectionFactorDescription">V</span>
                        </div>
                    </div>
                </div>

                <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.VoltageLoadCorrectionInfo')"></div>
            </CardElement>

            <CardElement :text="$t('powerlimiteradmin.InverterRestart')" textVariant="text-bg-primary" add-space
                         v-show="powerLimiterConfigList.enabled"
            >
                <div class="row mb-3" v-show="powerLimiterConfigList.enabled">
                    <label for="inputTimezone" class="col-sm-2 col-form-label">
                        {{ $t('powerlimiteradmin.InverterRestartHour') }}:
                        <BIconInfoCircle v-tooltip :title="$t('powerlimiteradmin.InverterRestartHint')" />
                    </label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="powerLimiterConfigList.inverter_restart_hour">
                            <option v-for="hour in restartHourList" :key="hour.key" :value="hour.key">
                                {{ hour.value }}
                            </option>
                        </select>
                    </div>
                </div>
            </CardElement>

            <FormFooter @reload="getPowerLimiterConfig"/>
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
import type { PowerLimiterConfig } from "@/types/PowerLimiterConfig";

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
            inverterList: [
                { key: 0, value: "ID 00" },
                { key: 1, value: "ID 01" },
                { key: 2, value: "ID 02" },
                { key: 3, value: "ID 03" },
                { key: 4, value: "ID 04" },
                { key: 5, value: "ID 05" },
                { key: 6, value: "ID 06" },
                { key: 7, value: "ID 07" },
                { key: 8, value: "ID 08" },
                { key: 9, value: "ID 09" },
                { key: 10, value: "ID 10" },
            ],
            inverterChannelList: [
                { key: 0, value: "CH 0" },
                { key: 1, value: "CH 1" },
                { key: 2, value: "CH 2" },
                { key: 3, value: "CH 3" },
            ],
            batteryDrainStrategyList: [
                { key: 0, value: "powerlimiteradmin.BatteryDrainWhenFull"},
                { key: 1, value: "powerlimiteradmin.BatteryDrainAtNight" },
            ],
            restartHourList: [
                { key: -1, value: "- - - -" },
                { key: 0, value: "0:00" },
                { key: 1, value: "1:00" },
                { key: 2, value: "2:00" },
                { key: 3, value: "3:00" },
                { key: 4, value: "4:00" },
                { key: 5, value: "5:00" },
                { key: 6, value: "6:00" },
                { key: 7, value: "7:00" },
                { key: 8, value: "8:00" },
                { key: 9, value: "9:00" },
                { key: 10, value: "10:00" },
                { key: 11, value: "11:00" },
                { key: 12, value: "12:00" },
                { key: 13, value: "13:00" },
                { key: 14, value: "14:00" },
                { key: 15, value: "15:00" },
                { key: 16, value: "16:00" },
                { key: 17, value: "17:00" },
                { key: 18, value: "18:00" },
                { key: 19, value: "19:00" },
                { key: 20, value: "20:00" },
                { key: 21, value: "21:00" },
                { key: 22, value: "22:00" },
                { key: 23, value: "23:00" },
            ],
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getPowerLimiterConfig();
    },
    methods: {
        getPowerLimiterConfig() {
            this.dataLoading = true;
            fetch("/api/powerlimiter/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.powerLimiterConfigList = data;
                    this.dataLoading = false;
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
