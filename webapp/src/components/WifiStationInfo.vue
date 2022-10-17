<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            WiFi Information (Station)
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Status</th>
                            <td class="badge" :class="{
                                'bg-danger': !sta_status,
                                'bg-success': sta_status,
                            }">
                                <span v-if="sta_status">enabled</span>
                                <span v-else>disabled</span>
                            </td>
                        </tr>
                        <tr>
                            <th>SSID</th>
                            <td>{{ sta_ssid }}</td>
                        </tr>
                        <tr>
                            <th>Quality</th>
                            <td>{{ getRSSIasQuality(sta_rssi) }} %</td>
                        </tr>
                        <tr>
                            <th>RSSI</th>
                            <td>{{ sta_rssi }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    props: {
        sta_status: { type: Boolean, required: true },
        sta_ssid: String,
        sta_rssi: { type: Number, required: true },
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
