<template>
    <BasePage :title="$t('gridchargeradmin.ChargerSettings')" :isLoading="dataLoading">
        <BootstrapAlert
            v-model="showAlert"
            dismissible
            :variant="alertType"
            :auto-dismiss="alertType != 'success' ? 0 : 5000"
        >
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveChargerConfig">
            <CardElement :text="$t('gridchargeradmin.Configuration')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('gridchargeradmin.EnableGridCharger')"
                    v-model="gridChargerConfigList.enabled"
                    type="checkbox"
                    wide
                />

                <template v-if="gridChargerConfigList.enabled">
                    <div class="row mb-3">
                        <label class="col-sm-4 col-form-label">
                            {{ $t('gridchargeradmin.Provider') }}
                        </label>
                        <div class="col-sm-8">
                            <select class="form-select" v-model="gridChargerConfigList.provider">
                                <option v-for="provider in providerTypeList" :key="provider.key" :value="provider.key">
                                    {{ $t(`gridchargeradmin.Provider` + provider.value) }}
                                </option>
                            </select>
                        </div>
                    </div>

                    <template v-if="gridChargerConfigList.provider === 0">
                        <div class="row mb-3">
                            <label class="col-sm-4 col-form-label">
                                {{ $t('gridchargeradmin.HardwareInterface') }}
                            </label>
                            <div class="col-sm-8">
                                <select class="form-select" v-model="gridChargerConfigList.can.hardware_interface">
                                    <option v-for="type in hardwareInterfaceList" :key="type.key" :value="type.key">
                                        {{ $t('gridchargeradmin.HardwareInterface' + type.value) }}
                                    </option>
                                </select>
                            </div>
                        </div>

                        <div class="row mb-3" v-if="gridChargerConfigList.can.hardware_interface === 0">
                            <label class="col-sm-4 col-form-label">
                                {{ $t('gridchargeradmin.CanControllerFrequency') }}
                            </label>
                            <div class="col-sm-8">
                                <select class="form-select" v-model="gridChargerConfigList.can.controller_frequency">
                                    <option
                                        v-for="frequency in frequencyTypeList"
                                        :key="frequency.key"
                                        :value="frequency.value"
                                    >
                                        {{ frequency.key }} MHz
                                    </option>
                                </select>
                            </div>
                        </div>

                        <InputElement
                            :label="$t('gridchargeradmin.EnableAutoPower')"
                            v-model="gridChargerConfigList.auto_power_enabled"
                            type="checkbox"
                            wide
                        />

                        <InputElement
                            v-if="gridChargerConfigList.auto_power_enabled"
                            :label="$t('gridchargeradmin.EnableBatterySoCLimits')"
                            v-model="gridChargerConfigList.auto_power_batterysoc_limits_enabled"
                            type="checkbox"
                            wide
                        />

                        <InputElement
                            :label="$t('gridchargeradmin.EnableEmergencyCharge')"
                            :tooltip="$t('gridchargeradmin.EnableEmergencyChargeHint')"
                            v-model="gridChargerConfigList.emergency_charge_enabled"
                            type="checkbox"
                            wide
                        />
                    </template>
                </template>
            </CardElement>

            <CardElement
                :text="$t('gridchargeradmin.HuaweiSettings')"
                textVariant="text-bg-primary"
                add-space
                v-if="gridChargerConfigList.enabled && gridChargerConfigList.provider === 0"
            >
                <InputElement
                    :label="$t('gridchargeradmin.OfflineVoltage')"
                    v-model="gridChargerConfigList.huawei.offline_voltage"
                    postfix="V"
                    type="number"
                    wide
                    step="0.01"
                />

                <InputElement
                    :label="$t('gridchargeradmin.OfflineCurrent')"
                    v-model="gridChargerConfigList.huawei.offline_current"
                    postfix="A"
                    type="number"
                    wide
                    step="0.1"
                />

                <InputElement
                    :label="$t('gridchargeradmin.InputCurrentLimit')"
                    v-model="gridChargerConfigList.huawei.input_current_limit"
                    postfix="A"
                    type="number"
                    wide
                    step="0.1"
                    :tooltip="$t('gridchargeradmin.InputCurrentLimitHint')"
                />

                <InputElement
                    :label="$t('gridchargeradmin.FanOnlineFullSpeed')"
                    v-model="gridChargerConfigList.huawei.fan_online_full_speed"
                    type="checkbox"
                    wide
                />

                <InputElement
                    :label="$t('gridchargeradmin.FanOfflineFullSpeed')"
                    v-model="gridChargerConfigList.huawei.fan_offline_full_speed"
                    type="checkbox"
                    wide
                />
            </CardElement>

            <template v-if="gridChargerConfigList.provider === 0">
                <CardElement
                    :text="$t('gridchargeradmin.Limits')"
                    textVariant="text-bg-primary"
                    add-space
                    v-if="
                        gridChargerConfigList.enabled &&
                        (gridChargerConfigList.auto_power_enabled || gridChargerConfigList.emergency_charge_enabled)
                    "
                >
                    <InputElement
                        :label="$t('gridchargeradmin.VoltageLimit')"
                        :tooltip="$t('gridchargeradmin.stopVoltageLimitHint')"
                        v-model="gridChargerConfigList.voltage_limit"
                        postfix="V"
                        type="number"
                        wide
                        required
                        step="0.01"
                        min="42"
                        max="58.5"
                    />

                    <InputElement
                        :label="$t('gridchargeradmin.enableVoltageLimit')"
                        :tooltip="$t('gridchargeradmin.enableVoltageLimitHint')"
                        v-model="gridChargerConfigList.enable_voltage_limit"
                        v-if="gridChargerConfigList.auto_power_enabled"
                        postfix="V"
                        type="number"
                        wide
                        required
                        step="0.01"
                        min="42"
                        max="58.5"
                    />

                    <InputElement
                        :label="$t('gridchargeradmin.lowerPowerLimit')"
                        v-model="gridChargerConfigList.lower_power_limit"
                        v-if="gridChargerConfigList.auto_power_enabled"
                        postfix="W"
                        type="number"
                        wide
                        required
                        min="50"
                        max="4000"
                    />

                    <InputElement
                        :label="$t('gridchargeradmin.upperPowerLimit')"
                        :tooltip="$t('gridchargeradmin.upperPowerLimitHint')"
                        v-model="gridChargerConfigList.upper_power_limit"
                        postfix="W"
                        type="number"
                        wide
                        required
                        min="100"
                        max="4000"
                    />

                    <InputElement
                        :label="$t('gridchargeradmin.targetPowerConsumption')"
                        :tooltip="$t('gridchargeradmin.targetPowerConsumptionHint')"
                        v-model="gridChargerConfigList.target_power_consumption"
                        v-if="gridChargerConfigList.auto_power_enabled"
                        postfix="W"
                        type="number"
                        wide
                        required
                    />
                </CardElement>

                <CardElement
                    :text="$t('gridchargeradmin.BatterySoCLimits')"
                    textVariant="text-bg-primary"
                    add-space
                    v-if="
                        gridChargerConfigList.enabled &&
                        gridChargerConfigList.auto_power_enabled &&
                        gridChargerConfigList.auto_power_batterysoc_limits_enabled
                    "
                >
                    <InputElement
                        :label="$t('gridchargeradmin.StopBatterySoCThreshold')"
                        :tooltip="$t('gridchargeradmin.StopBatterySoCThresholdHint')"
                        v-model="gridChargerConfigList.stop_batterysoc_threshold"
                        postfix="%"
                        type="number"
                        wide
                        required
                        min="2"
                        max="99"
                    />
                </CardElement>
            </template>

            <FormFooter @reload="getChargerConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { GridChargerConfig } from '@/types/GridChargerConfig';
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
            gridChargerConfigList: {} as GridChargerConfig,
            alertMessage: '',
            alertType: 'info',
            showAlert: false,
            providerTypeList: [{ key: 0, value: 'Huawei' }],
            frequencyTypeList: [
                { key: 8, value: 8000000 },
                { key: 16, value: 16000000 },
            ],
            hardwareInterfaceList: [
                { key: 0, value: 'MCP2515' },
                { key: 1, value: 'TWAI' },
            ],
        };
    },
    created() {
        this.getChargerConfig();
    },
    methods: {
        getChargerConfig() {
            this.dataLoading = true;
            fetch('/api/gridcharger/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.gridChargerConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveChargerConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.gridChargerConfigList));

            fetch('/api/gridcharger/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    this.alertMessage = this.$t('onbatteryapiresponse.' + response.code, response.param);
                    this.alertType = response.type;
                    this.showAlert = true;
                    window.scrollTo(0, 0);
                });
        },
    },
});
</script>
