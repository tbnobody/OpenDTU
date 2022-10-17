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
            <WifiStationInfo :networkStatus="networkDataList" />
            <div class="mt-5"></div>
            <WifiApInfo :networkStatus="networkDataList" />
            <div class="mt-5"></div>
            <InterfaceNetworkInfo :networkStatus="networkDataList" />
            <div class="mt-5"></div>
            <InterfaceApInfo :networkStatus="networkDataList" />
            <div class="mt-5"></div>
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import WifiStationInfo from "@/components/WifiStationInfo.vue";
import WifiApInfo from "@/components/WifiApInfo.vue";
import InterfaceNetworkInfo from "@/components/InterfaceNetworkInfo.vue";
import InterfaceApInfo from "@/components/InterfaceApInfo.vue";
import type { NetworkStatus } from '@/types/NetworkStatus';

export default defineComponent({
    components: {
        WifiStationInfo,
        WifiApInfo,
        InterfaceNetworkInfo,
        InterfaceApInfo,
    },
    data() {
        return {
            dataLoading: true,
            networkDataList: {} as NetworkStatus,
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