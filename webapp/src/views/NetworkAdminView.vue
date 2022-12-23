<template>
    <BasePage :title="$t('networkadmin.NetworkSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveNetworkConfig">
            <div class="card">
                <div class="card-header text-bg-primary">{{ $t('networkadmin.WifiConfiguration') }}</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputSSID" class="col-sm-2 col-form-label">
                            {{ $t('networkadmin.WifiSsid') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputSSID" maxlength="32"
                                v-model="networkConfigList.ssid" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPassword" class="col-sm-2 col-form-label">
                            {{ $t('networkadmin.WifiPassword') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="password" class="form-control" id="inputPassword" maxlength="64"
                                v-model="networkConfigList.password" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputHostname" class="col-sm-2 col-form-label">
                            {{ $t('networkadmin.Hostname') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputHostname" maxlength="32"
                                v-model="networkConfigList.hostname" />

                            <div class="alert alert-secondary" role="alert" v-html="$t('networkadmin.HostnameHint')">
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputDHCP">
                            {{ $t('networkadmin.EnableDhcp') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputDHCP"
                                    v-model="networkConfigList.dhcp" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="card" v-show="!networkConfigList.dhcp">
                <div class="card-header text-bg-primary">
                    {{ $t('networkadmin.StaticIpConfiguration') }}
                </div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputIP" class="col-sm-2 col-form-label">{{ $t('networkadmin.IpAddress') }}</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputIP" maxlength="32"
                                v-model="networkConfigList.ipaddress" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputNetmask" class="col-sm-2 col-form-label">
                            {{ $t('networkadmin.Netmask') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputNetmask" maxlength="32"
                                v-model="networkConfigList.netmask" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputGateway" class="col-sm-2 col-form-label">
                            {{ $t('networkadmin.DefaultGateway') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputGateway" maxlength="32"
                                v-model="networkConfigList.gateway" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputDNS1" class="col-sm-2 col-form-label">
                            {{ $t('networkadmin.Dns', { num: 1 }) }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputDNS1" maxlength="32"
                                v-model="networkConfigList.dns1" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputDNS2" class="col-sm-2 col-form-label">
                            {{ $t('networkadmin.Dns', { num: 2 }) }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputDNS2" maxlength="32"
                                v-model="networkConfigList.dns2" />
                        </div>
                    </div>
                </div>
            </div>
            <button type="submit" class="btn btn-primary mb-3">{{ $t('networkadmin.Save') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import type { NetworkConfig } from "@/types/NetworkkConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
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