<template>
    <div class="container" role="main">
        <div class="page-header">
            <h1>NTP Info</h1>
        </div>

        <div class="card">
            <div class="card-header text-white bg-primary">Configuration Summary</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Server</th>
                                <td>{{ ntpDataList.ntp_server }}</td>
                            </tr>
                            <tr>
                                <th>Timezone</th>
                                <td>{{ ntpDataList.ntp_timezone }}</td>
                            </tr>
                            <tr>
                                <th>Timezone Description</th>
                                <td>{{ ntpDataList.ntp_timezone_descr }}</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>

        <div class="card mt-5">
            <div class="card-header text-white bg-primary">Current Time</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Status</th>
                                <td class="badge" :class="{
                                    'bg-danger': !ntpDataList.ntp_status,
                                    'bg-success': ntpDataList.ntp_status,
                                }">
                                    <span v-if="ntpDataList.ntp_status">synced</span>
                                    <span v-else>not synced</span>
                                </td>
                            </tr>
                            <tr>
                                <th>Local Time</th>
                                <td>{{ ntpDataList.ntp_localtime }}</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    </div>
</template>

<script>
import { defineComponent } from 'vue';

export default defineComponent({
    data() {
        return {
            ntpDataList: [],
        };
    },
    created() {
        this.getNtpInfo();
    },
    methods: {
        getNtpInfo() {
            fetch("/api/ntp/status")
                .then((response) => response.json())
                .then((data) => (this.ntpDataList = data));
        },
    },
});
</script>