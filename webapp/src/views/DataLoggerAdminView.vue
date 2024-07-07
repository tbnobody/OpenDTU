<template>
    <BasePage :title="$t('dataloggeradmin.DataLoggerSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveDataLoggerConfig">
            <CardElement :text="$t('dataloggeradmin.DataLoggerConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('dataloggeradmin.EnableLogger')"
                               v-model="dataLoggerConfig.datalogger_enabled"
                               type="checkbox" />

                <InputElement v-show="dataLoggerConfig.datalogger_enabled"
                                :label="$t('dataloggeradmin.SaveInterval')"
                                v-model="dataLoggerConfig.saveinterval"
                                type="number" min="1" max="86400"
                                :postfix="$t('dataloggeradmin.Seconds')"/>

                <InputElement v-show="dataLoggerConfig.datalogger_enabled"
                                :label="$t('dataloggeradmin.FileName')"
                                v-model="dataLoggerConfig.filename"
                                type="input" :prefix="'/'"
                                :tooltip="$t('dataloggeradmin.FileNameHint')"/>
            </CardElement>

            <CardElement v-show="dataLoggerConfig.datalogger_enabled" :text="$t('dataloggeradmin.OutputConfiguration')" textVariant="text-bg-primary" addSpace>
                <InputElement :label="$t('invertertotalinfo.TotalYieldTotal')"
                               v-model="dataLoggerConfig.output_config.total_yield_total"
                               type="checkbox" wide />

                <InputElement :label="$t('invertertotalinfo.TotalYieldDay')"
                               v-model="dataLoggerConfig.output_config.total_yield_day"
                               type="checkbox" wide />

                <InputElement :label="$t('invertertotalinfo.TotalPower')"
                               v-model="dataLoggerConfig.output_config.total_power"
                               type="checkbox" wide />
            </CardElement>
            <FormFooter @reload="getDataLoggerConfig"/>
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { DataLoggerConfig } from "@/types/DataLoggerConfig";
import type { DeviceConfig } from "@/types/DeviceConfig";
import type { PinMapping, Device } from "@/types/PinMapping";
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
            deviceConfigList: {} as DeviceConfig,
            pinMappingList: {} as PinMapping,
            dataLoggerConfig: {} as DataLoggerConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getDataLoggerConfig();
    },
    computed: {
    },
    watch: {
    },
    methods: {
        getDataLoggerConfig() {
            this.dataLoading = true;

            fetch("/api/datalogger/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (data) => {
                        this.dataLoggerConfig = data;

                        fetch("/api/device/config", { headers: authHeader() })
                            .then((response) => handleResponse(response, this.$emitter, this.$router))
                            .then((data) => {
                                this.deviceConfigList = data;
                                const currentPinMapping = this.deviceConfigList.curPin;
                                console.log(this.deviceConfigList);

                                if(!currentPinMapping?.sd || !this.isValidSdConfig(currentPinMapping)) {
                                    this.alertMessage = this.$t('dataloggeradmin.SdConfigError');
                                    this.alertType = 'danger';
                                    this.showAlert = true;
                                }
                            }).catch((error) => {
                                if (error.status != 404) {
                                    this.alertMessage = this.$t('deviceadmin.ParseError', { error: error.message });
                                    this.alertType = 'danger';
                                    this.showAlert = true;
                                }
                                this.pinMappingList = Array<Device>();
                            }).finally(() => {
                                this.dataLoading = false;
                            });
                    }
                );
        },
        saveDataLoggerConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.dataLoggerConfig));

            fetch("/api/datalogger/config", {
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
        isValidSdConfig(pinMapping: Device) {
            return pinMapping?.sd?.clk >= 0
                && pinMapping?.sd?.cs >= 0
                && pinMapping?.sd?.miso >= 0
                && pinMapping?.sd?.mosi >= 0;
        },
    },
});
</script>
