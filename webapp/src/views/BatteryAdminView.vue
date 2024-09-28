<template>
    <BasePage :title="$t('batteryadmin.BatterySettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveBatteryConfig">
            <CardElement :text="$t('batteryadmin.BatteryConfiguration')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('batteryadmin.EnableBattery')"
                    v-model="batteryConfigList.enabled"
                    type="checkbox"
                />

                <InputElement
                    v-show="batteryConfigList.enabled"
                    :label="$t('batteryadmin.VerboseLogging')"
                    v-model="batteryConfigList.verbose_logging"
                    type="checkbox"
                />

                <div class="row mb-3" v-show="batteryConfigList.enabled">
                    <label class="col-sm-2 col-form-label">
                        {{ $t('batteryadmin.Provider') }}
                    </label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="batteryConfigList.provider">
                            <option v-for="provider in providerTypeList" :key="provider.key" :value="provider.key">
                                {{ $t(`batteryadmin.Provider` + provider.value) }}
                            </option>
                        </select>
                    </div>
                </div>
            </CardElement>

            <CardElement
                v-show="batteryConfigList.enabled && batteryConfigList.provider == 1"
                :text="$t('batteryadmin.JkBmsConfiguration')"
                textVariant="text-bg-primary"
                addSpace
            >
                <div class="row mb-3">
                    <label class="col-sm-2 col-form-label">
                        {{ $t('batteryadmin.JkBmsInterface') }}
                    </label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="batteryConfigList.jkbms_interface">
                            <option
                                v-for="jkBmsInterface in jkBmsInterfaceTypeList"
                                :key="jkBmsInterface.key"
                                :value="jkBmsInterface.key"
                            >
                                {{ $t(`batteryadmin.JkBmsInterface` + jkBmsInterface.value) }}
                            </option>
                        </select>
                    </div>
                </div>

                <InputElement
                    :label="$t('batteryadmin.PollingInterval')"
                    v-model="batteryConfigList.jkbms_polling_interval"
                    type="number"
                    min="2"
                    max="90"
                    step="1"
                    :postfix="$t('batteryadmin.Seconds')"
                />
            </CardElement>

            <template v-if="batteryConfigList.enabled && batteryConfigList.provider == 2">
                <CardElement :text="$t('batteryadmin.MqttSocConfiguration')" textVariant="text-bg-primary" addSpace>
                    <InputElement
                        :label="$t('batteryadmin.MqttSocTopic')"
                        v-model="batteryConfigList.mqtt_soc_topic"
                        type="text"
                        maxlength="256"
                    />

                    <InputElement
                        :label="$t('batteryadmin.MqttJsonPath')"
                        v-model="batteryConfigList.mqtt_soc_json_path"
                        type="text"
                        maxlength="128"
                        :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                    />
                </CardElement>

                <CardElement :text="$t('batteryadmin.MqttVoltageConfiguration')" textVariant="text-bg-primary" addSpace>
                    <InputElement
                        :label="$t('batteryadmin.MqttVoltageTopic')"
                        v-model="batteryConfigList.mqtt_voltage_topic"
                        type="text"
                        maxlength="256"
                    />

                    <InputElement
                        :label="$t('batteryadmin.MqttJsonPath')"
                        v-model="batteryConfigList.mqtt_voltage_json_path"
                        type="text"
                        maxlength="128"
                        :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                    />

                    <div class="row mb-3">
                        <label for="mqtt_voltage_unit" class="col-sm-2 col-form-label">
                            {{ $t('batteryadmin.MqttVoltageUnit') }}
                        </label>
                        <div class="col-sm-10">
                            <select
                                id="mqtt_voltage_unit"
                                class="form-select"
                                v-model="batteryConfigList.mqtt_voltage_unit"
                            >
                                <option v-for="u in voltageUnitTypeList" :key="u.key" :value="u.key">
                                    {{ u.value }}
                                </option>
                            </select>
                        </div>
                    </div>
                </CardElement>
            </template>

            <CardElement
                :text="$t('batteryadmin.DischargeCurrentLimitConfiguration')"
                textVariant="text-bg-primary"
                addSpace
            >
                <InputElement
                    :label="$t('batteryadmin.LimitDischargeCurrent')"
                    v-model="batteryConfigList.enable_discharge_current_limit"
                    type="checkbox"
                />

                <template v-if="batteryConfigList.enable_discharge_current_limit">
                    <InputElement
                        :label="$t('batteryadmin.DischargeCurrentLimit')"
                        v-model="batteryConfigList.discharge_current_limit"
                        type="number"
                        min="0"
                        step="0.1"
                        postfix="A"
                    />

                    <InputElement
                        :label="$t('batteryadmin.DischargeCurrentLimitBelowSoc')"
                        v-model="batteryConfigList.discharge_current_limit_below_soc"
                        type="number"
                        min="0"
                        max="100"
                        step="0.1"
                        postfix="%"
                        :tooltip="$t('batteryadmin.DischargeCurrentLimitBelowSocInfo')"
                    />

                    <InputElement
                        :label="$t('batteryadmin.DischargeCurrentLimitBelowVoltage')"
                        v-model="batteryConfigList.discharge_current_limit_below_voltage"
                        type="number"
                        min="0"
                        max="60"
                        step="0.01"
                        postfix="V"
                        :tooltip="$t('batteryadmin.DischargeCurrentLimitBelowVoltageInfo')"
                    />

                    <InputElement
                        :label="$t('batteryadmin.UseBatteryReportedDischargeCurrentLimit')"
                        v-model="batteryConfigList.use_battery_reported_discharge_current_limit"
                        type="checkbox"
                    />
                </template>

                <template
                    v-if="
                        batteryConfigList.enable_discharge_current_limit &&
                        batteryConfigList.use_battery_reported_discharge_current_limit
                    "
                >
                    <div
                        class="alert alert-secondary"
                        role="alert"
                        v-html="$t('batteryadmin.BatteryReportedDischargeCurrentLimitInfo')"
                    ></div>

                    <template v-if="batteryConfigList.provider == 2">
                        <InputElement
                            :label="$t('batteryadmin.MqttDischargeCurrentTopic')"
                            v-model="batteryConfigList.mqtt_discharge_current_topic"
                            type="text"
                            maxlength="256"
                        />

                        <InputElement
                            :label="$t('batteryadmin.MqttJsonPath')"
                            v-model="batteryConfigList.mqtt_discharge_current_json_path"
                            type="text"
                            maxlength="128"
                            :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                        />

                        <div class="row mb-3">
                            <label for="mqtt_amperage_unit" class="col-sm-2 col-form-label">
                                {{ $t('batteryadmin.MqttAmperageUnit') }}
                            </label>

                            <div class="col-sm-10">
                                <select
                                    id="mqtt_amperage_unit"
                                    class="form-select"
                                    v-model="batteryConfigList.mqtt_amperage_unit"
                                >
                                    <option v-for="u in amperageUnitTypeList" :key="u.key" :value="u.key">
                                        {{ u.value }}
                                    </option>
                                </select>
                            </div>
                        </div>
                    </template>
                </template>
            </CardElement>

            <FormFooter @reload="getBatteryConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { BatteryConfig } from '@/types/BatteryConfig';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
        InputElement,
    },
    data() {
        return {
            dataLoading: true,
            batteryConfigList: {} as BatteryConfig,
            alertMessage: '',
            alertType: 'info',
            showAlert: false,
            providerTypeList: [
                { key: 0, value: 'PylontechCan' },
                { key: 1, value: 'JkBmsSerial' },
                { key: 2, value: 'Mqtt' },
                { key: 3, value: 'Victron' },
                { key: 4, value: 'PytesCan' },
                { key: 5, value: 'SBSCan' },
            ],
            jkBmsInterfaceTypeList: [
                { key: 0, value: 'Uart' },
                { key: 1, value: 'Transceiver' },
            ],
            voltageUnitTypeList: [
                { key: 3, value: 'mV' },
                { key: 2, value: 'cV' },
                { key: 1, value: 'dV' },
                { key: 0, value: 'V' },
            ],
            amperageUnitTypeList: [
                { key: 1, value: 'mA' },
                { key: 0, value: 'A' },
            ],
        };
    },
    created() {
        this.getBatteryConfig();
    },
    methods: {
        getBatteryConfig() {
            this.dataLoading = true;
            fetch('/api/battery/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.batteryConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveBatteryConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.batteryConfigList));

            fetch('/api/battery/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                    this.alertType = response.type;
                    this.showAlert = true;
                });
        },
    },
});
</script>
