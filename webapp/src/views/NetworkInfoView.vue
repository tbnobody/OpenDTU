<template>
    <BasePage :title="'Network Info'" :isLoading="dataLoading">
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
import { defineComponent } from 'vue';
import { handleResponse, authHeader } from '@/utils/authentication';
import BasePage from '@/components/BasePage.vue';
import WifiStationInfo from "@/components/WifiStationInfo.vue";
import WifiApInfo from "@/components/WifiApInfo.vue";
import InterfaceNetworkInfo from "@/components/InterfaceNetworkInfo.vue";
import InterfaceApInfo from "@/components/InterfaceApInfo.vue";
import type { NetworkStatus } from '@/types/NetworkStatus';

export default defineComponent({
    components: {
        BasePage,
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