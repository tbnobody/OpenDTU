<template>
    <BasePage :title="$t('acchargeradmin.ChargerSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveChargerConfig">
            <CardElement :text="$t('acchargeradmin.Configuration')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('acchargeradmin.EnableHuawei')"
                    v-model="acChargerConfigList.enabled"
                    type="checkbox"
                    wide
                />

                <template v-if="acChargerConfigList.enabled">
                    <div class="row mb-3">
                        <label class="col-sm-4 col-form-label">
                            {{ $t('acchargeradmin.CanControllerFrequency') }}
                        </label>
                        <div class="col-sm-8">
                            <select class="form-select" v-model="acChargerConfigList.can_controller_frequency">
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
                        :label="$t('acchargeradmin.VerboseLogging')"
                        v-model="acChargerConfigList.verbose_logging"
                        type="checkbox"
                        wide
                    />

                    <InputElement
                        :label="$t('acchargeradmin.EnableAutoPower')"
                        v-model="acChargerConfigList.auto_power_enabled"
                        type="checkbox"
                        wide
                    />

                    <InputElement
                        v-if="acChargerConfigList.auto_power_enabled"
                        :label="$t('acchargeradmin.EnableBatterySoCLimits')"
                        v-model="acChargerConfigList.auto_power_batterysoc_limits_enabled"
                        type="checkbox"
                        wide
                    />

                    <InputElement
                        :label="$t('acchargeradmin.EnableEmergencyCharge')"
                        :tooltip="$t('acchargeradmin.EnableEmergencyChargeHint')"
                        v-model="acChargerConfigList.emergency_charge_enabled"
                        type="checkbox"
                        wide
                    />
                </template>
            </CardElement>

            <CardElement
                :text="$t('acchargeradmin.Limits')"
                textVariant="text-bg-primary"
                add-space
                v-if="acChargerConfigList.auto_power_enabled || acChargerConfigList.emergency_charge_enabled"
            >
                <InputElement
                    :label="$t('acchargeradmin.VoltageLimit')"
                    :tooltip="$t('acchargeradmin.stopVoltageLimitHint')"
                    v-model="acChargerConfigList.voltage_limit"
                    postfix="V"
                    type="number"
                    wide
                    required
                    step="0.01"
                    min="42"
                    max="58.5"
                />

                <InputElement
                    :label="$t('acchargeradmin.enableVoltageLimit')"
                    :tooltip="$t('acchargeradmin.enableVoltageLimitHint')"
                    v-model="acChargerConfigList.enable_voltage_limit"
                    postfix="V"
                    type="number"
                    wide
                    required
                    step="0.01"
                    min="42"
                    max="58.5"
                />

                <InputElement
                    :label="$t('acchargeradmin.lowerPowerLimit')"
                    v-model="acChargerConfigList.lower_power_limit"
                    postfix="W"
                    type="number"
                    wide
                    required
                    min="50"
                    max="3000"
                />

                <InputElement
                    :label="$t('acchargeradmin.upperPowerLimit')"
                    :tooltip="$t('acchargeradmin.upperPowerLimitHint')"
                    v-model="acChargerConfigList.upper_power_limit"
                    postfix="W"
                    type="number"
                    wide
                    required
                    min="100"
                    max="3000"
                />

                <InputElement
                    :label="$t('acchargeradmin.targetPowerConsumption')"
                    :tooltip="$t('acchargeradmin.targetPowerConsumptionHint')"
                    v-model="acChargerConfigList.target_power_consumption"
                    postfix="W"
                    type="number"
                    wide
                    required
                />
            </CardElement>
            <CardElement
                :text="$t('acchargeradmin.BatterySoCLimits')"
                textVariant="text-bg-primary"
                add-space
                v-if="
                    acChargerConfigList.auto_power_enabled && acChargerConfigList.auto_power_batterysoc_limits_enabled
                "
            >
                <InputElement
                    :label="$t('acchargeradmin.StopBatterySoCThreshold')"
                    :tooltip="$t('acchargeradmin.StopBatterySoCThresholdHint')"
                    v-model="acChargerConfigList.stop_batterysoc_threshold"
                    postfix="%"
                    type="number"
                    wide
                    required
                    min="2"
                    max="99"
                />
            </CardElement>

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
import type { AcChargerConfig } from '@/types/AcChargerConfig';
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
            acChargerConfigList: {} as AcChargerConfig,
            alertMessage: '',
            alertType: 'info',
            showAlert: false,
            frequencyTypeList: [
                { key: 8, value: 8000000 },
                { key: 16, value: 16000000 },
            ],
        };
    },
    created() {
        this.getChargerConfig();
    },
    methods: {
        getChargerConfig() {
            this.dataLoading = true;
            fetch('/api/huawei/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.acChargerConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveChargerConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.acChargerConfigList));

            fetch('/api/huawei/config', {
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
