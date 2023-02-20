<template>
    <BasePage :title="'Dynamic Power limiter Settings'" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="savePowerLimiterConfig">
            <div class="card">
                <div class="card-header text-bg-primary">{{ $t('powerlimiteradmin.General') }}</div>
                <div class="card-body">

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputPowerlimiter">{{ $t('powerlimiteradmin.Enable') }}</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputPowerlimiter"
                                    v-model="powerLimiterConfigList.enabled" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="solarPasstroughEnabled">{{ $t('powerlimiteradmin.EnableSolarPasstrough') }}</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="solarPasstroughEnabled"
                                    v-model="powerLimiterConfigList.solar_passtrough_enabled" />
                            </div>
                        </div>
                    </div>

                    <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.SolarpasstroughInfo')"></div>

                    <div class="row mb-3">
                        <label for="inputLowerPowerLimit" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.LowerPowerLimit') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="inputLowerPowerLimit"
                                    placeholder="50" min="10" v-model="powerLimiterConfigList.lower_power_limit"
                                    aria-describedby="lowerPowerLimitDescription" />
                                    <span class="input-group-text" id="lowerPowerLimitDescription">W</span>
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputUpperPowerLimit" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.UpperPowerLimit') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="inputUpperPowerLimit"
                                    placeholder="800" v-model="powerLimiterConfigList.upper_power_limit"
                                    aria-describedby="upperPowerLimitDescription" />
                                <span class="input-group-text" id="upperPowerLimitDescription">W</span>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="card mt-5">
                <div class="card-header text-bg-primary">{{ $t('powerlimiteradmin.PowerMeters') }}</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputMqttTopicPowerMeter1" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.MqttTopicPowerMeter1') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="text" class="form-control" id="inputMqttTopicPowerMeter1"
                                    placeholder="shellies/shellyem3/emeter/0/power" v-model="powerLimiterConfigList.mqtt_topic_powermeter_1" />
                            </div>
                        </div>
                    </div>
                
                    <div class="row mb-3">
                        <label for="inputMqttTopicPowerMeter2" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.MqttTopicPowerMeter2') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="text" class="form-control" id="inputMqttTopicPowerMeter2"
                                    placeholder="shellies/shellyem3/emeter/1/power" v-model="powerLimiterConfigList.mqtt_topic_powermeter_2" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputMqttTopicPowerMeter3" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.MqttTopicPowerMeter3') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="text" class="form-control" id="inputMqttTopicPowerMeter3"
                                    placeholder="shellies/shellyem3/emeter/2/power" v-model="powerLimiterConfigList.mqtt_topic_powermeter_3" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputRetain">{{ $t('powerlimiteradmin.InverterIsBehindPowerMeter') }}</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputIsInverterBehindPowerMeter"
                                    v-model="powerLimiterConfigList.is_inverter_behind_powermeter" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="card mt-5">
                <div class="card-header text-bg-primary">{{ $t('powerlimiteradmin.Battery') }}</div>
                <div class="card-body">

                    <div class="row mb-3">
                        <label for="batterySocStartThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.BatterySocStartThreshold') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="batterySocStartThreshold"
                                    placeholder="80" v-model="powerLimiterConfigList.battery_soc_start_threshold"
                                    aria-describedby="batterySocStartThresholdDescription" min="0" max="100" />
                                    <span class="input-group-text" id="batterySocStartThresholdDescription">%</span>
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="batterySocStopThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.BatterySocStopThreshold') }}</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="batterySocStopThreshold"
                                    placeholder="20" v-model="powerLimiterConfigList.battery_soc_stop_threshold"
                                    aria-describedby="batterySocStopThresholdDescription" min="0" max="100" />
                                    <span class="input-group-text" id="batterySocStopThresholdDescription">%</span>
                            </div>
                        </div>
                    </div>

                    <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.BatterySocInfo')"></div>

                    <div class="row mb-3">
                        <label for="inputVoltageStartThreshold" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.VoltageStartThreshold') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" step="0.01" class="form-control" id="inputVoltageStartThreshold"
                                    placeholder="50" v-model="powerLimiterConfigList.voltage_start_threshold"
                                    aria-describedby="voltageStartThresholdDescription" />
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
                                    aria-describedby="voltageStopThresholdDescription" />
                                <span class="input-group-text" id="voltageStopThresholdDescription">V</span>
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputVoltageLoadCorrectionFactor" class="col-sm-2 col-form-label">{{ $t('powerlimiteradmin.VoltageLoadCorrectionFactor') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" step="0.0001" class="form-control" id="inputVoltageLoadCorrectionFactor"
                                    placeholder="49" v-model="powerLimiterConfigList.voltage_load_correction_factor"
                                    aria-describedby="voltageLoadCorrectionFactorDescription" />
                                <span class="input-group-text" id="voltageLoadCorrectionFactorDescription">V</span>
                            </div>
                        </div>
                    </div>

                    <div class="alert alert-secondary" role="alert" v-html="$t('powerlimiteradmin.VoltageLoadCorrectionInfo')"></div>
                </div>
            </div>

            <button type="submit" class="btn btn-primary mb-3">{{ $t('powerlimiteradmin.Save') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import type { PowerLimiterConfig } from "@/types/PowerLimiterConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: true,
            powerLimiterConfigList: {} as PowerLimiterConfig,
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
