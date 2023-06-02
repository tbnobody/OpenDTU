<template>
    <BasePage :title="$t('networkinfo.NetworkInformation')" :isLoading="dataLoading" :show-reload="true" @reload="getNetworkInfo">
        <WifiStationInfo :networkStatus="networkDataList" />
        <div class="mt-5"></div>
        <WifiApInfo :networkStatus="networkDataList" />
        <div class="mt-5"></div>
        <InterfaceNetworkInfo :networkStatus="networkDataList" />
        <div class="mt-5"></div>
        <InterfaceApInfo :networkStatus="networkDataList" />
        <div class="mt-5"></div>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import InterfaceApInfo from "@/components/InterfaceApInfo.vue";
import InterfaceNetworkInfo from "@/components/InterfaceNetworkInfo.vue";
import WifiApInfo from "@/components/WifiApInfo.vue";
import WifiStationInfo from "@/components/WifiStationInfo.vue";
import type { NetworkStatus } from '@/types/NetworkStatus';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        InterfaceApInfo,
        InterfaceNetworkInfo,
        WifiApInfo,
        WifiStationInfo,
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
            fetch("/api/network/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.networkDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>