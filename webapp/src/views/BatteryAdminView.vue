<template>
    <BasePage :title="$t('batteryadmin.BatterySettings')" :isLoading="dataLoading">
        <BootstrapAlert
            v-model="showAlert"
            dismissible
            :variant="alertType"
            :auto-dismiss="alertType != 'success' ? 0 : 5000"
        >
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveBatteryConfig">
            <CardElement :text="$t('batteryadmin.BatteryConfiguration')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('batteryadmin.EnableBattery')"
                    v-model="batteryConfigList.enabled"
                    type="checkbox"
                    wide
                />

                <template v-if="batteryConfigList.enabled">
                    <div class="row mb-3">
                        <label class="col-sm-4 col-form-label">
                            {{ $t('batteryadmin.Provider') }}
                        </label>
                        <div class="col-sm-8">
                            <select class="form-select" v-model="batteryConfigList.provider">
                                <option v-for="provider in providerTypeList" :key="provider.key" :value="provider.key">
                                    {{ $t(`batteryadmin.Provider` + provider.value) }}
                                </option>
                            </select>
                        </div>
                    </div>
                </template>
            </CardElement>

            <CardElement
                v-if="batteryConfigList.enabled && (batteryConfigList.provider == 1 || batteryConfigList.provider == 6)"
                :text="$t('batteryadmin.SerialSettings')"
                textVariant="text-bg-primary"
                addSpace
            >
                <div class="row mb-3">
                    <label class="col-sm-4 col-form-label">
                        {{ $t('batteryadmin.SerialInterfaceType') }}
                    </label>
                    <div class="col-sm-8">
                        <select class="form-select" v-model="batteryConfigList.serial.interface">
                            <option
                                v-for="serialInterface in serialBmsInterfaceTypeList"
                                :key="serialInterface.key"
                                :value="serialInterface.key"
                            >
                                {{ $t(`batteryadmin.SerialInterfaceType` + serialInterface.value) }}
                            </option>
                        </select>
                    </div>
                </div>

                <InputElement
                    :label="$t('batteryadmin.PollingInterval')"
                    v-model="batteryConfigList.serial.polling_interval"
                    type="number"
                    min="2"
                    max="90"
                    step="1"
                    :postfix="$t('batteryadmin.Seconds')"
                    wide
                />
            </CardElement>

            <template v-if="batteryConfigList.enabled && batteryConfigList.provider == 2">
                <CardElement :text="$t('batteryadmin.MqttSocConfiguration')" textVariant="text-bg-primary" addSpace>
                    <InputElement
                        :label="$t('batteryadmin.MqttSocTopic')"
                        v-model="batteryConfigList.mqtt.soc_topic"
                        type="text"
                        maxlength="256"
                        wide
                    />

                    <InputElement
                        :label="$t('batteryadmin.MqttJsonPath')"
                        v-model="batteryConfigList.mqtt.soc_json_path"
                        type="text"
                        maxlength="256"
                        :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                        wide
                    />
                </CardElement>

                <CardElement :text="$t('batteryadmin.MqttVoltageConfiguration')" textVariant="text-bg-primary" addSpace>
                    <InputElement
                        :label="$t('batteryadmin.MqttVoltageTopic')"
                        v-model="batteryConfigList.mqtt.voltage_topic"
                        type="text"
                        maxlength="256"
                        wide
                    />

                    <InputElement
                        :label="$t('batteryadmin.MqttJsonPath')"
                        v-model="batteryConfigList.mqtt.voltage_json_path"
                        type="text"
                        maxlength="256"
                        :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                        wide
                    />

                    <div class="row mb-3">
                        <label for="mqtt_voltage_unit" class="col-sm-4 col-form-label">
                            {{ $t('batteryadmin.MqttVoltageUnit') }}
                        </label>
                        <div class="col-sm-8">
                            <select
                                id="mqtt_voltage_unit"
                                class="form-select"
                                v-model="batteryConfigList.mqtt.voltage_unit"
                            >
                                <option v-for="u in voltageUnitTypeList" :key="u.key" :value="u.key">
                                    {{ u.value }}
                                </option>
                            </select>
                        </div>
                    </div>
                </CardElement>

                <CardElement :text="$t('batteryadmin.MqttCurrentConfiguration')" textVariant="text-bg-primary" addSpace>
                    <InputElement
                        :label="$t('batteryadmin.MqttCurrentTopic')"
                        v-model="batteryConfigList.mqtt.current_topic"
                        type="text"
                        maxlength="256"
                        wide
                    />

                    <InputElement
                        :label="$t('batteryadmin.MqttJsonPath')"
                        v-model="batteryConfigList.mqtt.current_json_path"
                        type="text"
                        maxlength="256"
                        :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                        wide
                    />

                    <div class="row mb-3">
                        <label for="mqtt_current_unit" class="col-sm-4 col-form-label">
                            {{ $t('batteryadmin.MqttAmperageUnit') }}
                        </label>
                        <div class="col-sm-8">
                            <select
                                id="mqtt_current_unit"
                                class="form-select"
                                v-model="batteryConfigList.mqtt.current_unit"
                            >
                                <option v-for="u in amperageUnitTypeList" :key="u.key" :value="u.key">
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
                    wide
                />

                <template v-if="batteryConfigList.enable_discharge_current_limit">
                    <InputElement
                        :label="$t('batteryadmin.DischargeCurrentLimit')"
                        v-model="batteryConfigList.discharge_current_limit"
                        type="number"
                        min="0"
                        step="0.1"
                        postfix="A"
                        wide
                    />

                    <InputElement
                        :label="$t('batteryadmin.DischargeCurrentLimitBelowSoc')"
                        v-if="batteryConfigList.enabled"
                        v-model="batteryConfigList.discharge_current_limit_below_soc"
                        type="number"
                        min="0"
                        max="100"
                        step="0.1"
                        postfix="%"
                        :tooltip="$t('batteryadmin.DischargeCurrentLimitBelowSocInfo')"
                        wide
                    />

                    <InputElement
                        :label="$t('batteryadmin.DischargeCurrentLimitBelowVoltage')"
                        v-if="batteryConfigList.enabled"
                        v-model="batteryConfigList.discharge_current_limit_below_voltage"
                        type="number"
                        min="0"
                        max="60"
                        step="0.01"
                        postfix="V"
                        :tooltip="$t('batteryadmin.DischargeCurrentLimitBelowVoltageInfo')"
                        wide
                    />

                    <template
                        v-if="
                            batteryConfigList.enabled &&
                            (batteryConfigList.provider == 0 ||
                                batteryConfigList.provider == 2 ||
                                batteryConfigList.provider == 4 ||
                                batteryConfigList.provider == 5)
                        "
                    >
                        <InputElement
                            :label="$t('batteryadmin.UseBatteryReportedDischargeCurrentLimit')"
                            v-model="batteryConfigList.use_battery_reported_discharge_current_limit"
                            type="checkbox"
                            wide
                        />

                        <template v-if="batteryConfigList.use_battery_reported_discharge_current_limit">
                            <div
                                class="alert alert-secondary"
                                role="alert"
                                v-if="batteryConfigList.enabled"
                                v-html="$t('batteryadmin.BatteryReportedDischargeCurrentLimitInfo')"
                            ></div>

                            <template v-if="batteryConfigList.provider == 2">
                                <InputElement
                                    :label="$t('batteryadmin.MqttDischargeCurrentLimitTopic')"
                                    v-model="batteryConfigList.mqtt.discharge_current_limit_topic"
                                    wide
                                    type="text"
                                    maxlength="256"
                                />

                                <InputElement
                                    :label="$t('batteryadmin.MqttJsonPath')"
                                    v-model="batteryConfigList.mqtt.discharge_current_limit_json_path"
                                    wide
                                    type="text"
                                    maxlength="256"
                                    :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                                />

                                <div class="row mb-3">
                                    <label for="discharge_current_limit_unit" class="col-sm-4 col-form-label">
                                        {{ $t('batteryadmin.MqttAmperageUnit') }}
                                    </label>

                                    <div class="col-sm-8">
                                        <select
                                            id="discharge_current_limit_unit"
                                            class="form-select"
                                            v-model="batteryConfigList.mqtt.discharge_current_limit_unit"
                                        >
                                            <option v-for="u in amperageUnitTypeList" :key="u.key" :value="u.key">
                                                {{ u.value }}
                                            </option>
                                        </select>
                                    </div>
                                </div>
                            </template>
                        </template>
                    </template>
                </template>
            </CardElement>

            <template v-if="batteryConfigList.enabled && batteryConfigList.provider == 7">
                <CardElement :text="$t('batteryadmin.ZendureConfiguration')" textVariant="text-bg-primary" addSpace>
                    <div class="row mb-3">
                        <label for="zendure_device_type" class="col-sm-2 col-form-label">
                            {{ $t('batteryadmin.ZendureDeviceType') }}
                        </label>
                        <div class="col-sm-10">
                            <select
                                id="zendure_device_type"
                                class="form-select"
                                v-model="batteryConfigList.zendure.device_type"
                            >
                                <option v-for="u in zendureDeviceTypeList" :key="u.key" :value="u.key">
                                    {{ $t('batteryadmin.zendure.deviceTypes.' + u.value) }}
                                </option>
                            </select>
                        </div>
                    </div>

                    <InputElement
                        :label="$t('batteryadmin.ZendureDeviceId')"
                        v-model="batteryConfigList.zendure.device_id"
                        type="text"
                        minlength="8"
                        maxlength="8"
                        :tooltip="$t('batteryadmin.ZendureDeviceIdDescription')"
                    />

                    <InputElement
                        :label="$t('batteryadmin.PollingInterval')"
                        v-model="batteryConfigList.zendure.polling_interval"
                        type="number"
                        min="10"
                        max="120"
                        step="1"
                        :postfix="$t('batteryadmin.Seconds')"
                    />

                    <div class="row mb-3">
                        <label for="zendure_control_mode" class="col-sm-2 col-form-label">
                            {{ $t('batteryadmin.zendure.controlMode') }}
                        </label>
                        <div class="col-sm-10">
                            <select
                                id="zendure_control_mode"
                                class="form-select"
                                v-model="batteryConfigList.zendure.control_mode"
                                @change="
                                    batteryConfigList.zendure.output_control = 0;
                                    batteryConfigList.zendure.charge_through_enable = false;
                                "
                            >
                                <option v-for="u in zendureControlModeList" :key="u.key" :value="u.key">
                                    {{ $t('batteryadmin.zendure.controlModes.' + u.value) }}
                                </option>
                            </select>
                        </div>
                    </div>

                    <template
                        v-if="
                            batteryConfigList.zendure.control_mode == 0 || batteryConfigList.zendure.control_mode == 1
                        "
                    >
                        <InputElement
                            :label="$t('batteryadmin.ZendureMaxOutput')"
                            v-model="batteryConfigList.zendure.max_output"
                            type="number"
                            min="100"
                            max="1200"
                            step="100"
                            :postfix="$t('batteryadmin.Watt')"
                        />

                        <InputElement
                            :label="$t('batteryadmin.ZendureMinSoc')"
                            v-model="batteryConfigList.zendure.soc_min"
                            type="number"
                            min="0"
                            max="60"
                            step="1"
                            :postfix="$t('batteryadmin.Percent')"
                        />

                        <InputElement
                            :label="$t('batteryadmin.ZendureMaxSoc')"
                            v-model="batteryConfigList.zendure.soc_max"
                            type="number"
                            min="40"
                            max="100"
                            step="1"
                            :postfix="$t('batteryadmin.Percent')"
                        />

                        <div class="row mb-3">
                            <label for="zendure_bypass_mode" class="col-sm-2 col-form-label">
                                {{ $t('batteryadmin.ZendureBypassMode') }}
                            </label>
                            <div class="col-sm-10">
                                <select
                                    id="zendure_bypass_mode"
                                    class="form-select"
                                    v-model="batteryConfigList.zendure.bypass_mode"
                                >
                                    <option v-for="u in zendureBypassModeList" :key="u.key" :value="u.key">
                                        {{ $t(`batteryadmin.ZendureBypassMode` + u.value) }}
                                    </option>
                                </select>
                            </div>
                        </div>

                        <InputElement
                            :label="$t('batteryadmin.ZendureAutoShutdown')"
                            v-model="batteryConfigList.zendure.auto_shutdown"
                            type="checkbox"
                            :tooltip="$t('batteryadmin.ZendureAutoShutdownDescription')"
                        />

                        <InputElement
                            :label="$t('batteryadmin.zendure.buzzerEnable')"
                            v-model="batteryConfigList.zendure.buzzer_enable"
                            type="checkbox"
                        />
                    </template>
                </CardElement>

                <template
                    v-if="batteryConfigList.zendure.control_mode == 0 && batteryConfigList.zendure.output_control != 0"
                >
                    <CardElement
                        :text="$t('batteryadmin.zendure.chargeThrough')"
                        textVariant="text-bg-primary"
                        addSpace
                    >
                        <InputElement
                            :label="$t('batteryadmin.zendure.chargeThroughEnabled')"
                            v-model="batteryConfigList.zendure.charge_through_enable"
                            type="checkbox"
                        />
                        <template v-if="batteryConfigList.zendure.charge_through_enable">
                            <InputElement
                                :label="$t('batteryadmin.zendure.chargeThroughInterval')"
                                v-model="batteryConfigList.zendure.charge_through_interval"
                                type="number"
                                min="0"
                                max="8766"
                                step="1"
                                :postfix="$t('batteryadmin.Hours')"
                            />
                            <InputElement
                                :label="$t('batteryadmin.zendure.chargeThroughReset')"
                                v-model="batteryConfigList.zendure.charge_through_reset"
                                type="number"
                                min="25"
                                max="100"
                                step="1"
                                :postfix="$t('batteryadmin.Percent')"
                            />
                        </template>
                    </CardElement>
                </template>

                <CardElement :text="$t('batteryadmin.ZendureOutputControl')" textVariant="text-bg-primary" addSpace>
                    <div class="row mb-3">
                        <label for="zendure_output_mode" class="col-sm-2 col-form-label">
                            {{ $t('batteryadmin.Mode') }}
                        </label>
                        <div class="col-sm-10">
                            <select
                                id="zendure_output_mode"
                                class="form-select"
                                v-model="batteryConfigList.zendure.output_control"
                                @change="batteryConfigList.zendure.charge_through_enable = false"
                            >
                                <option :key="0" :value="0">
                                    {{ $t('batteryadmin.ZendureOutputMode' + zendureOutputControlList[0]?.value) }}
                                </option>
                                <option
                                    :key="1"
                                    :value="1"
                                    v-if="
                                        batteryConfigList.zendure.control_mode == 0 ||
                                        batteryConfigList.zendure.control_mode == 1
                                    "
                                >
                                    {{ $t('batteryadmin.ZendureOutputMode' + zendureOutputControlList[1]?.value) }}
                                </option>
                                <option :key="2" :value="2" v-if="batteryConfigList.zendure.control_mode == 0">
                                    {{ $t('batteryadmin.ZendureOutputMode' + zendureOutputControlList[2]?.value) }}
                                </option>
                            </select>
                        </div>
                    </div>

                    <template v-if="batteryConfigList.zendure.output_control == 1">
                        <InputElement
                            :label="$t('batteryadmin.ZendureOutputLimit')"
                            v-model="batteryConfigList.zendure.output_limit"
                            type="number"
                            min="0"
                            max="1200"
                            step="1"
                            :postfix="$t('batteryadmin.Watt')"
                        />
                    </template>

                    <template v-if="batteryConfigList.zendure.output_control == 2">
                        <InputElement
                            :label="$t('batteryadmin.ZendureSunriseOffset')"
                            v-model="batteryConfigList.zendure.sunrise_offset"
                            type="number"
                            min="-360"
                            max="360"
                            step="1"
                            :postfix="$t('batteryadmin.Minutes')"
                        />
                        <InputElement
                            :label="$t('batteryadmin.ZendureOutputLimitDay')"
                            v-model="batteryConfigList.zendure.output_limit_day"
                            type="number"
                            min="0"
                            max="1200"
                            step="1"
                            :postfix="$t('batteryadmin.Watt')"
                        />
                        <InputElement
                            :label="$t('batteryadmin.ZendureSunsetOffset')"
                            v-model="batteryConfigList.zendure.sunset_offset"
                            type="number"
                            min="-360"
                            max="360"
                            step="1"
                            :postfix="$t('batteryadmin.Minutes')"
                        />
                        <InputElement
                            :label="$t('batteryadmin.ZendureOutputLimitNight')"
                            v-model="batteryConfigList.zendure.output_limit_night"
                            type="number"
                            min="0"
                            max="1200"
                            step="1"
                            :postfix="$t('batteryadmin.Watt')"
                        />
                    </template>
                </CardElement>
            </template>

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
                { key: 6, value: 'JbdBmsSerial' },
                { key: 7, value: 'ZendureMqtt' },
            ],
            serialBmsInterfaceTypeList: [
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
            zendureDeviceTypeList: [
                { key: 0, value: 'Hub1200' },
                { key: 1, value: 'Hub2000' },
                { key: 2, value: 'AIO2400' },
                { key: 3, value: 'Ace1500' },
                { key: 4, value: 'Hyper2000A' },
                { key: 5, value: 'Hyper2000B' },
            ],
            zendureBypassModeList: [
                { key: 0, value: 'Automatic' },
                { key: 1, value: 'AlwaysOff' },
                { key: 2, value: 'AlwaysOn' },
            ],
            zendureOutputControlList: [
                { key: 0, value: 'External' },
                { key: 1, value: 'Fixed' },
                { key: 2, value: 'Schedule' },
            ],
            zendureControlModeList: [
                { key: 0, value: 'Full' },
                { key: 1, value: 'Once' },
                { key: 2, value: 'ReadOnly' },
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
                    window.scrollTo(0, 0);
                });
        },
    },
});
</script>
