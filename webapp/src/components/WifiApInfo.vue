<template>
    <div class="card">
        <div class="card-header text-bg-primary">
            WiFi Information (Access Point)
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Status</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !networkStatus.ap_status,
                                'text-bg-success': networkStatus.ap_status,
                            }">
                                <span v-if="networkStatus.ap_status">enabled</span>
                                <span v-else>disabled</span>
                            </td>
                        </tr>
                        <tr>
                            <th>SSID</th>
                            <td>{{ networkStatus.ap_ssid }}</td>
                        </tr>
                        <tr>
                            <th># Stations</th>
                            <td>{{ networkStatus.ap_stationnum }}</td>
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
});
</script>
