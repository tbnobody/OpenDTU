<template>
    <BasePage :title="$t('wireguardadmin.WireguardSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>
        <form @submit="saveWireguardConfig">
            <CardElement :text="$t('wireguardadmin.WireguardConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('wireguardadmin.EnableWireguard')"
                              v-model="wireguardConfigList.wg_enabled"
                              type="checkbox" wide/>
            </CardElement>
            <CardElement :text="$t('wireguardadmin.WireguardParameters')" textVariant="text-bg-primary"
                        v-show="wireguardConfigList.wg_enabled"
            >
                <InputElement :label="$t('wireguardadmin.EndpointAddress')"
                              v-model="wireguardConfigList.wg_endpoint_address"
                              type="text" maxlength="128"
                              :placeholder="$t('wireguardadmin.EndpointAddressHint')"/>
                <InputElement :label="$t('wireguardadmin.EndpointPort')"
                              v-model="wireguardConfigList.wg_endpoint_port"
                              type="text" maxlength="128"
                              :placeholder="$t('wireguardadmin.EndpointPortHint')"/>
                <InputElement :label="$t('wireguardadmin.EndpointPublicKey')"
                              v-model="wireguardConfigList.wg_endpoint_public_key"
                              type="text" maxlength="128"
                              :placeholder="$t('wireguardadmin.EndpointPublicKeyHint')"/>
                <InputElement :label="$t('wireguardadmin.OpenDTULocalIP')"
                              v-model="wireguardConfigList.wg_opendtu_local_ip"
                              type="text" maxlength="128"
                              :placeholder="$t('wireguardadmin.OpenDTULocalIPHint')"/>
                <InputElement :label="$t('wireguardadmin.OpenDTUAllowedIP')"
                              v-model="wireguardConfigList.wg_opendtu_allowed_ip"
                              type="text" maxlength="128"
                              :placeholder="$t('wireguardadmin.OpenDTUAllowedIPHint')"/>
                <InputElement :label="$t('wireguardadmin.OpenDTUAllowedMask')"
                              v-model="wireguardConfigList.wg_opendtu_allowed_mask"
                              type="text" maxlength="128"
                              :placeholder="$t('wireguardadmin.OpenDTUAllowedMaskHint')"/>
                <InputElement :label="$t('wireguardadmin.OpenDTUPublicKey')"
                              v-model="wireguardConfigList.wg_opendtu_public_key"
                              type="text" maxlength="128"
                              :placeholder="$t('wireguardadmin.OpenDTUPublicKeyHint')"/>
                <InputElement :label="$t('wireguardadmin.OpenDTUPrivateKey')"
                              v-model="wireguardConfigList.wg_opendtu_private_key"
                              type="text" maxlength="128"
                              :placeholder="$t('wireguardadmin.OpenDTUPrivateKeyHint')"/>
            </CardElement>
            <button type="submit" class="btn btn-danger">{{ $t('wireguardadmin.SaveReboot') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import type { WireguardConfig } from "@/types/WireguardConfig";
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
            wireguardConfigList: {} as WireguardConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getMqttConfig();
    },
    methods: {
        getMqttConfig() {
            this.dataLoading = true;
            fetch("/api/wireguard/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.wireguardConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveWireguardConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.wireguardConfigList));

            fetch("/api/wireguard/config", {
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
