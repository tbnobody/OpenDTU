<template>
    <BasePage :title="$t('batteryadmin.BatterySettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveBatteryConfig">
            <CardElement :text="$t('batteryadmin.BatteryConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('batteryadmin.EnableBattery')"
                              v-model="batteryConfigList.enabled"
                              type="checkbox" />

                <InputElement v-show="batteryConfigList.enabled"
                                :label="$t('batteryadmin.VerboseLogging')"
                                v-model="batteryConfigList.verbose_logging"
                                type="checkbox"/>

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

            <CardElement v-show="batteryConfigList.enabled && batteryConfigList.provider == 1"
                         :text="$t('batteryadmin.JkBmsConfiguration')" textVariant="text-bg-primary" addSpace>
                <div class="row mb-3">
                    <label class="col-sm-2 col-form-label">
                        {{ $t('batteryadmin.JkBmsInterface') }}
                    </label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="batteryConfigList.jkbms_interface">
                            <option v-for="jkBmsInterface in jkBmsInterfaceTypeList" :key="jkBmsInterface.key" :value="jkBmsInterface.key">
                                {{ $t(`batteryadmin.JkBmsInterface` + jkBmsInterface.value) }}
                            </option>
                        </select>
                    </div>
                </div>

                <InputElement :label="$t('batteryadmin.PollingInterval')"
                              v-model="batteryConfigList.jkbms_polling_interval"
                              type="number" min="2" max="90" step="1" :postfix="$t('batteryadmin.Seconds')"/>
            </CardElement>

            <CardElement v-show="batteryConfigList.enabled && batteryConfigList.provider == 2"
                         :text="$t('batteryadmin.MqttConfiguration')" textVariant="text-bg-primary" addSpace>
                <div class="row mb-3">
                    <label class="col-sm-2 col-form-label">
                        {{ $t('batteryadmin.MqttTopic') }}
                    </label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input type="text" class="form-control" v-model="batteryConfigList.mqtt_topic" />
                        </div>
                    </div>
                </div>
            </CardElement>

            <FormFooter @reload="getBatteryConfig"/>
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { BatteryConfig } from "@/types/BatteryConfig";
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
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            providerTypeList: [
                { key: 0, value: 'PylontechCan' },
                { key: 1, value: 'JkBmsSerial' },
                { key: 2, value: 'Mqtt' },
                { key: 3, value: 'Victron' },
            ],
            jkBmsInterfaceTypeList: [
                { key: 0, value: 'Uart' },
                { key: 1, value: 'Transceiver' },
            ],
        };
    },
    created() {
        this.getBatteryConfig();
    },
    methods: {
        getBatteryConfig() {
            this.dataLoading = true;
            fetch("/api/battery/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.batteryConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveBatteryConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.batteryConfigList));

            fetch("/api/battery/config", {
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
