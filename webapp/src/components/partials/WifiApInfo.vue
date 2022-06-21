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
                                'bg-danger': !networkDataList.ap_status,
                                'bg-success': networkDataList.ap_status,
                            }">
                                <span v-if="networkDataList.ap_status">enabled</span>
                                <span v-else>disabled</span>
                            </td>
                        </tr>
                        <tr>
                            <th>SSID</th>
                            <td>{{ networkDataList.ap_ssid }}</td>
                        </tr>
                        <tr>
                            <th># Stations</th>
                            <td>{{ networkDataList.ap_stationnum }}</td>
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
    data() {
        return {
            networkDataList: {
                ap_status: false,
                ap_ssid: "",
                ap_stationnum: 0
            },
        };
    },
    created() {
        this.getNetworkInfo();
    },
    methods: {
        getNetworkInfo() {
            fetch("/api/network/status")
                .then((response) => response.json())
                .then((data) => (this.networkDataList = data));
        },
    },
});
</script>
