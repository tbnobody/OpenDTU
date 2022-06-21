<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            Firmware Information
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Hostname</th>
                            <td>{{ systemDataList.hostname }}</td>
                        </tr>
                        <tr>
                            <th>SDK Version</th>
                            <td>{{ systemDataList.sdkversion }}</td>
                        </tr>
                        <tr>
                            <th>Firmware Version</th>
                            <td>{{ systemDataList.firmware_version }}</td>
                        </tr>
                        <tr>
                            <th>Git Hash</th>
                            <td>{{ systemDataList.git_hash }}</td>
                        </tr>
                        <tr>
                            <th>Reset Reason CPU 0</th>
                            <td>{{ systemDataList.resetreason_0 }}</td>
                        </tr>
                        <tr>
                            <th>Reset Reason CPU 1</th>
                            <td>{{ systemDataList.resetreason_1 }}</td>
                        </tr>
                        <tr>
                            <th>Config save count</th>
                            <td>{{ systemDataList.cfgsavecount }}</td>
                        </tr>
                        <tr>
                            <th>Uptime</th>
                            <td>{{ timeInHours(systemDataList.uptime) }}</td>
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
            systemDataList: {
                hostname: "",
                sdkversion: "",
                firmware_version: "",
                git_hash: "",
                resetreason_0: "",
                resetreason_1: "",
                cfgsavecount: 0,
                uptime: 0
            },
        };
    },
    created() {
        this.getSystemInfo();
    },
    computed: {
        timeInHours() {
            return (value: number) => {
                const days = Math.floor(value / 3600 / 24);
                const hours = Math.floor((value - days * 3600 * 24) / 3600);
                const minutes = Math.floor((value - days * 3600 * 24 - hours * 3600) / 60);
                const seconds = (value - days * 3600 * 24 - hours * 3600 + minutes * 60) % 60;

                const dHours = hours > 9 ? hours : "0" + hours;
                const dMins = minutes > 9 ? minutes : "0" + minutes;
                const dSecs = seconds > 9 ? seconds : "0" + seconds;

                return days + " days " + dHours + ":" + dMins + ":" + dSecs;
            };
        },
    },
    methods: {
        getSystemInfo() {
            fetch("/api/system/status")
                .then((response) => response.json())
                .then((data) => (this.systemDataList = data));
        },
    },
});
</script>
