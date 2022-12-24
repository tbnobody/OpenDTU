<template>
    <CardElement :text="$t('wifiapinfo.WifiApInfo')" textVariant="text-bg-primary">
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('wifiapinfo.Status') }}</th>
                        <td class="badge" :class="{
                            'text-bg-danger': !networkStatus.ap_status,
                            'text-bg-success': networkStatus.ap_status,
                        }">
                            <span v-if="networkStatus.ap_status">{{ $t('wifiapinfo.Enabled') }}</span>
                            <span v-else>{{ $t('wifiapinfo.Disabled') }}</span>
                        </td>
                    </tr>
                    <tr>
                        <th>{{ $t('wifiapinfo.Ssid') }}</th>
                        <td>{{ networkStatus.ap_ssid }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('wifiapinfo.Stations') }}</th>
                        <td>{{ networkStatus.ap_stationnum }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import type { NetworkStatus } from '@/types/NetworkStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        networkStatus: { type: Object as PropType<NetworkStatus>, required: true },
    },
});
</script>
