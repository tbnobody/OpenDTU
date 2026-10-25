<template>
    <BasePage :title="$t('networkinfo.NetworkInformation')" :show-reload="true" @reload="refreshKey++">
        <NetworkConnectionStatus :refresh-key="refreshKey" @status="networkDataList = $event" />
        <template v-if="networkDataList">
            <div class="mt-5"></div>
            <WifiStationInfo :networkStatus="networkDataList" />
            <div class="mt-5"></div>
            <WifiApInfo :networkStatus="networkDataList" />
            <div class="mt-5"></div>
            <InterfaceNetworkInfo :networkStatus="networkDataList" />
            <div class="mt-5"></div>
            <InterfaceApInfo :networkStatus="networkDataList" />
            <div class="mt-5"></div>
        </template>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import InterfaceApInfo from '@/components/InterfaceApInfo.vue';
import InterfaceNetworkInfo from '@/components/InterfaceNetworkInfo.vue';
import NetworkConnectionStatus from '@/components/NetworkConnectionStatus.vue';
import WifiApInfo from '@/components/WifiApInfo.vue';
import WifiStationInfo from '@/components/WifiStationInfo.vue';
import type { NetworkStatus } from '@/types/NetworkStatus';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        InterfaceApInfo,
        InterfaceNetworkInfo,
        NetworkConnectionStatus,
        WifiApInfo,
        WifiStationInfo,
    },
    data() {
        return {
            refreshKey: 0,
            networkDataList: null as NetworkStatus | null,
        };
    },
});
</script>
