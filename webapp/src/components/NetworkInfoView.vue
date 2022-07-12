<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>Network Info</h1>
        </div>
        <div class="text-center" v-if="dataLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </div>

        <template v-if="!dataLoading">
            <WifiStationInfo v-bind="networkDataList" />
            <div class="mt-5"></div>
            <WifiApInfo v-bind="networkDataList" />
            <div class="mt-5"></div>
            <InterfaceStationInfo v-bind="networkDataList" />
            <div class="mt-5"></div>
            <InterfaceApInfo v-bind="networkDataList" />
            <div class="mt-5"></div>
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import WifiStationInfo from "./partials/WifiStationInfo.vue";
import WifiApInfo from "./partials/WifiApInfo.vue";
import InterfaceStationInfo from "./partials/InterfaceStationInfo.vue";
import InterfaceApInfo from "./partials/InterfaceApInfo.vue";

export default defineComponent({
    components: {
        WifiStationInfo,
        WifiApInfo,
        InterfaceStationInfo,
        InterfaceApInfo,
    },
    data() {
        return {
            dataLoading: true,
            networkDataList: {
                // WifiStationInfo
                sta_status: false,
                sta_ssid: "",
                sta_rssi: 0,
                // WifiApInfo
                ap_status: false,
                ap_ssid: "",
                ap_stationnum: 0,
                // InterfaceStationInfo
                sta_ip: "",
                sta_netmask: "",
                sta_gateway: "",
                sta_dns1: "",
                sta_dns2: "",
                sta_mac: "",
                // InterfaceApInfo
                ap_ip: "",
                ap_mac: "",
            }
        }
    },
    created() {
        this.getNetworkInfo();
    },
    methods: {
        getNetworkInfo() {
            this.dataLoading = true;
            fetch("/api/network/status")
                .then((response) => response.json())
                .then((data) => {
                    this.networkDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>