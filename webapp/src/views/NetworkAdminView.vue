<template>
    <BasePage :title="$t('networkadmin.NetworkSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveNetworkConfig">
            <CardElement :text="$t('networkadmin.WifiConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('networkadmin.WifiSsid')"
                              v-model="networkConfigList.ssid"
                              type="text" maxlength="32"/>

                <InputElement :label="$t('networkadmin.WifiPassword')"
                              v-model="networkConfigList.password"
                              type="password" maxlength="64"/>

                <InputElement :label="$t('networkadmin.Hostname')"
                              v-model="networkConfigList.hostname"
                              type="text" maxlength="32"
                >
                    <div class="alert alert-secondary" role="alert" v-html="$t('networkadmin.HostnameHint')"></div>
                </InputElement>

                <InputElement :label="$t('networkadmin.EnableDhcp')"
                              v-model="networkConfigList.dhcp"
                              type="checkbox"/>
            </CardElement>

            <CardElement :text="$t('networkadmin.StaticIpConfiguration')" textVariant="text-bg-primary" add-space
                         v-show="!networkConfigList.dhcp"
            >
                <InputElement :label="$t('networkadmin.IpAddress')"
                              v-model="networkConfigList.ipaddress"
                              type="text" maxlength="32"/>

                <InputElement :label="$t('networkadmin.Netmask')"
                              v-model="networkConfigList.netmask"
                              type="text" maxlength="32"/>

                <InputElement :label="$t('networkadmin.DefaultGateway')"
                              v-model="networkConfigList.gateway"
                              type="text" maxlength="32"/>

                <InputElement :label="$t('networkadmin.Dns', { num: 1 })"
                              v-model="networkConfigList.dns1"
                              type="text" maxlength="32"/>

                <InputElement :label="$t('networkadmin.Dns', { num: 2 })"
                              v-model="networkConfigList.dns2"
                              type="text" maxlength="32"/>
            </CardElement>

            <button type="submit" class="btn btn-primary mb-3">{{ $t('networkadmin.Save') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import type { NetworkConfig } from "@/types/NetworkkConfig";
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
            networkConfigList: {} as NetworkConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getNetworkConfig();
    },
    methods: {
        getNetworkConfig() {
            this.dataLoading = true;
            fetch("/api/network/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.networkConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveNetworkConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.networkConfigList));

            fetch("/api/network/config", {
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