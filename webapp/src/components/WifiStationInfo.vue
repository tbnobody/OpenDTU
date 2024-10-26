<template>
    <CardElement :text="$t('wifistationinfo.WifiStationInfo')" textVariant="text-bg-primary" table>
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('wifistationinfo.Status') }}</th>
                        <td>
                            <StatusBadge
                                :status="networkStatus.sta_status"
                                true_text="wifistationinfo.Enabled"
                                false_text="wifistationinfo.Disabled"
                            />
                        </td>
                    </tr>
                    <tr>
                        <th>{{ $t('wifistationinfo.Ssid') }}</th>
                        <td>{{ networkStatus.sta_ssid }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('wifistationinfo.Bssid') }}</th>
                        <td>{{ networkStatus.sta_bssid }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('wifistationinfo.Quality') }}</th>
                        <td>{{ $n(getRSSIasQuality(networkStatus.sta_rssi), 'percent') }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('wifistationinfo.Rssi') }}</th>
                        <td>{{ $n(networkStatus.sta_rssi, 'decimal') }}</td>
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
    methods: {
        getRSSIasQuality(rssi: number) {
            let quality = 0;

            if (rssi <= -100) {
                quality = 0;
            } else if (rssi >= -50) {
                quality = 100;
            } else {
                quality = 2 * (rssi + 100);
            }

            return quality / 100;
        },
    },
});
</script>
