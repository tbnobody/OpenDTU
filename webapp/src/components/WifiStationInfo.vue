<template>
    <div class="card">
        <div class="card-header text-bg-primary">
            WiFi Information (Station)
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Status</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !networkStatus.sta_status,
                                'text-bg-success': networkStatus.sta_status,
                            }">
                                <span v-if="networkStatus.sta_status">enabled</span>
                                <span v-else>disabled</span>
                            </td>
                        </tr>
                        <tr>
                            <th>SSID</th>
                            <td>{{ networkStatus.sta_ssid }}</td>
                        </tr>
                        <tr>
                            <th>Quality</th>
                            <td>{{ getRSSIasQuality(networkStatus.sta_rssi) }} %</td>
                        </tr>
                        <tr>
                            <th>RSSI</th>
                            <td>{{ networkStatus.sta_rssi }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import type { NetworkStatus } from '@/types/NetworkStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
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

            return quality;
        },
    },
});
</script>
