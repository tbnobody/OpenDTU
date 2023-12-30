<template>
    <BasePage :title="$t('acchargeradmin.ChargerSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveChargerConfig">
            <CardElement :text="$t('acchargeradmin.Configuration')" textVariant="text-bg-primary">
                <InputElement :label="$t('acchargeradmin.EnableHuawei')"
                              v-model="acChargerConfigList.enabled"
                              type="checkbox" wide/>

                <div class="row mb-3" v-show="acChargerConfigList.enabled">
                    <label class="col-sm-4 col-form-label">
                        {{ $t('acchargeradmin.CanControllerFrequency') }}
                    </label>
                    <div class="col-sm-8">
                        <select class="form-select" v-model="acChargerConfigList.can_controller_frequency">
                            <option v-for="frequency in frequencyTypeList" :key="frequency.key" :value="frequency.value">
                                {{ frequency.key }} MHz
                            </option>
                        </select>
                    </div>
                </div>

                <InputElement v-show="acChargerConfigList.enabled"
                              :label="$t('acchargeradmin.EnableAutoPower')"
                              v-model="acChargerConfigList.auto_power_enabled"
                              type="checkbox" wide/>

                <CardElement :text="$t('acchargeradmin.Limits')" textVariant="text-bg-primary" add-space
                              v-show="acChargerConfigList.auto_power_enabled">
                    <div class="row mb-3">
                        <label for="voltageLimit" class="col-sm-2 col-form-label">{{ $t('acchargeradmin.VoltageLimit') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" step="0.01" class="form-control" id="voltageLimit"
                                    placeholder="42" v-model="acChargerConfigList.voltage_limit"
                                    aria-describedby="voltageLimitDescription" min="42" max="58.5" required/>
                                    <span class="input-group-text" id="voltageLimitDescription">V</span>
                            </div>
                        </div>
                        <label for="enableVoltageLimit" class="col-sm-2 col-form-label">{{ $t('acchargeradmin.enableVoltageLimit') }}:
                          <BIconInfoCircle v-tooltip :title="$t('acchargeradmin.enableVoltageLimitHint')" />
                        </label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" step="0.01" class="form-control" id="enableVoltageLimit"
                                    placeholder="42" v-model="acChargerConfigList.enable_voltage_limit"
                                    aria-describedby="enableVoltageLimitDescription" min="42" max="58.5" required/>
                                    <span class="input-group-text" id="enableVoltageLimitDescription">V</span>
                            </div>
                        </div>
                        <label for="lowerPowerLimit" class="col-sm-2 col-form-label">{{ $t('acchargeradmin.lowerPowerLimit') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="lowerPowerLimit"
                                    placeholder="150" v-model="acChargerConfigList.lower_power_limit"
                                    aria-describedby="lowerPowerLimitDescription" min="100" max="3000" required/>
                                    <span class="input-group-text" id="lowerPowerLimitDescription">W</span>
                            </div>
                        </div>
                        <label for="upperPowerLimit" class="col-sm-2 col-form-label">{{ $t('acchargeradmin.upperPowerLimit') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="upperPowerLimit"
                                    placeholder="2000" v-model="acChargerConfigList.upper_power_limit"
                                    aria-describedby="lowerPowerLimitDescription" min="100" max="3000" required/>
                                    <span class="input-group-text" id="upperPowerLimitDescription">W</span>
                            </div>
                        </div>
                    </div>
                </CardElement>
            </CardElement>

            <FormFooter @reload="getChargerConfig"/>
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import { BIconInfoCircle } from 'bootstrap-icons-vue';
import type { AcChargerConfig } from "@/types/AcChargerConfig";
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

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
            acChargerConfigList: {} as AcChargerConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            frequencyTypeList: [
                { key:  8, value:  8000000 },
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
            fetch("/api/huawei/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.acChargerConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveChargerConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.acChargerConfigList));

            fetch("/api/huawei/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>
