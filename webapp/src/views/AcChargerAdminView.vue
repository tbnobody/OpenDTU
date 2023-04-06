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
            </CardElement>

            <button type="submit" class="btn btn-primary mb-3">{{ $t('acchargeradmin.Save') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import type { AcChargerConfig } from "@/types/AcChargerConfig";
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        InputElement,
    },
    data() {
        return {
            dataLoading: true,
            acChargerConfigList: {} as AcChargerConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
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
