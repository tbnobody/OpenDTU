<template>
    <CardElement :text="$t('wifiapinfo.WifiApInfo')" textVariant="text-bg-primary" table>
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('wifiapinfo.Status') }}</th>
                        <td>
                            <StatusBadge
                                :status="networkStatus.ap_status"
                                true_text="wifiapinfo.Enabled"
                                false_text="wifiapinfo.Disabled"
                            />
                        </td>
                    </tr>
                    <tr>
                        <th>{{ $t('wifiapinfo.Ssid') }}</th>
                        <td>{{ networkStatus.ap_ssid }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('wifiapinfo.Stations') }}</th>
                        <td>{{ $n(networkStatus.ap_stationnum, 'decimal') }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import StatusBadge from './StatusBadge.vue';
import type { NetworkStatus } from '@/types/NetworkStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
        StatusBadge,
    },
    props: {
        networkStatus: { type: Object as PropType<NetworkStatus>, required: true },
    },
});
</script>
