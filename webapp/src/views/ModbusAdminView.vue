<template>
    <BasePage :title="$t('modbusadmin.ModbusSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveModbusConfig">
            <CardElement :text="$t('modbusadmin.ModbusConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('modbusadmin.EnableModbusTCP')"
                              v-model="modbusConfigList.modbus_tcp_enabled"
                              type="checkbox" wide/>
                <InputElement :label="$t('modbusadmin.Clients')"
                              v-model="modbusConfigList.modbus_clients"
                              type="number" min="1" max="4"/>
                <InputElement :label="$t('modbusadmin.IDDTUPro')"
                              v-model="modbusConfigList.modbus_id_dtupro"
                              type="number" min="1" max="255"/>
                <InputElement :label="$t('modbusadmin.IDTotal')"
                              v-model="modbusConfigList.modbus_id_total"
                              type="number" min="1" max="255"/>
                <InputElement :label="$t('modbusadmin.IDMeter')"
                              v-model="modbusConfigList.modbus_id_meter"
                              type="number" min="1" max="255"/>
            </CardElement>

            <FormFooter @reload="getModbusConfig"/>
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { ModbusConfig } from "@/types/ModbusConfig";
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
            modbusConfigList: {} as ModbusConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getModbusConfig();
    },
    methods: {
        getModbusConfig() {
            this.dataLoading = true;
            fetch("/api/modbus/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.modbusConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveModbusConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.modbusConfigList));

            fetch("/api/modbus/config", {
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
