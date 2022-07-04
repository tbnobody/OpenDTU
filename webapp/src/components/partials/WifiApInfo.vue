<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            WiFi Information (Access Point)
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Status</th>
                            <td class="badge" :class="{
                                'bg-danger': !ap_status,
                                'bg-success': ap_status,
                            }">
                                <span v-if="ap_status">enabled</span>
                                <span v-else>disabled</span>
                            </td>
                        </tr>
                        <tr>
                            <th>SSID</th>
                            <td>{{ ap_ssid }}</td>
                        </tr>
                        <tr>
                            <th># Stations</th>
                            <td>{{ ap_stationnum }}</td>
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
        ap_status: { type: Boolean, required: true },
        ap_ssid: String,
        ap_stationnum: { type: Number, required: true },
    },
});
</script>
