<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>Network Settings</h1>
        </div>
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <div class="text-center" v-if="dataLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </div>

        <template v-if="!dataLoading">
            <form @submit="saveNetworkConfig">
                <div class="card">
                    <div class="card-header text-white bg-primary">WiFi Configuration</div>
                    <div class="card-body">
                        <div class="row mb-3">
                            <label for="inputSSID" class="col-sm-2 col-form-label">WiFi SSID:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputSSID" maxlength="32" placeholder="SSID"
                                    v-model="networkConfigList.ssid" />
                            </div>
                        </div>

                        <div class="row mb-3">
                            <label for="inputPassword" class="col-sm-2 col-form-label">WiFi Password:</label>
                            <div class="col-sm-10">
                                <input type="password" class="form-control" id="inputPassword" maxlength="64"
                                    placeholder="PSK" v-model="networkConfigList.password" />
                            </div>
                        </div>

                        <div class="row mb-3">
                            <label for="inputHostname" class="col-sm-2 col-form-label">Hostname:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputHostname" maxlength="32"
                                    placeholder="Hostname" v-model="networkConfigList.hostname" />

                                <div class="alert alert-secondary" role="alert">
                                    <b>Hint:</b> The text <span class="font-monospace">%06X</span> will be replaced
                                    with the last 6 digits of the ESP ChipID in hex format.
                                </div>
                            </div>
                        </div>

                        <div class="row mb-3">
                            <label class="col-sm-2 form-check-label" for="inputDHCP">Enable DHCP</label>
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
                    <div class="card-header text-white bg-primary">
                        Static IP Configuration
                    </div>
                    <div class="card-body">
                        <div class="row mb-3">
                            <label for="inputIP" class="col-sm-2 col-form-label">IP Address:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputIP" maxlength="32"
                                    placeholder="IP address" v-model="networkConfigList.ipaddress" />
                            </div>
                        </div>

                        <div class="row mb-3">
                            <label for="inputNetmask" class="col-sm-2 col-form-label">Netmask:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputNetmask" maxlength="32"
                                    placeholder="Netmask" v-model="networkConfigList.netmask" />
                            </div>
                        </div>

                        <div class="row mb-3">
                            <label for="inputGateway" class="col-sm-2 col-form-label">Default Gateway:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputGateway" maxlength="32"
                                    placeholder="Default Gateway" v-model="networkConfigList.gateway" />
                            </div>
                        </div>

                        <div class="row mb-3">
                            <label for="inputDNS1" class="col-sm-2 col-form-label">DNS Server 1:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputDNS1" maxlength="32"
                                    placeholder="DNS Server 1" v-model="networkConfigList.dns1" />
                            </div>
                        </div>

                        <div class="row mb-3">
                            <label for="inputDNS2" class="col-sm-2 col-form-label">DNS Server 2:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputDNS2" maxlength="32"
                                    placeholder="DNS Server 2" v-model="networkConfigList.dns2" />
                            </div>
                        </div>
                    </div>
                </div>
                <button type="submit" class="btn btn-primary mb-3">Save</button>
            </form>
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BootstrapAlert from "@/components/partials/BootstrapAlert.vue";

export default defineComponent({
    components: {
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: true,
            networkConfigList: {
                ssid: "",
                password: "",
                hostname: "",
                dhcp: false,
                ipaddress: "",
                netmask: "",
                gateway: "",
                dns1: "",
                dns2: ""
            },
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
            fetch("/api/network/config")
                .then((response) => response.json())
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
                body: formData,
            })
                .then(function (response) {
                    if (response.status != 200) {
                        throw response.status;
                    } else {
                        return response.json();
                    }
                })
                .then(
                    (response) => {
                        this.alertMessage = response.message;
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>