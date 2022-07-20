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
                            <td>{{ hostname }}</td>
                        </tr>
                        <tr>
                            <th>SDK Version</th>
                            <td>{{ sdkversion }}</td>
                        </tr>
                        <tr>
                            <th>Firmware Version</th>
                            <td>{{ firmware_version }}</td>
                        </tr>
                        <tr>
                            <th>Git Hash</th>
                            <td><a :href="'https://github.com/tbnobody/OpenDTU/commits/' + git_hash?.substring(1)" target="_blank">{{ git_hash?.substring(1) }}</a></td>
                        </tr>
                        <tr>
                            <th>Reset Reason CPU 0</th>
                            <td>{{ resetreason_0 }}</td>
                        </tr>
                        <tr>
                            <th>Reset Reason CPU 1</th>
                            <td>{{ resetreason_1 }}</td>
                        </tr>
                        <tr>
                            <th>Config save count</th>
                            <td>{{ cfgsavecount }}</td>
                        </tr>
                        <tr>
                            <th>Uptime</th>
                            <td>{{ timeInHours(uptime) }}</td>
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
        hostname: String,
        sdkversion: String,
        firmware_version: String,
        git_hash: String,
        resetreason_0: String,
        resetreason_1: String,
        cfgsavecount: { type: Number, required: true },
        uptime: { type: Number, required: true },
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
});
</script>
