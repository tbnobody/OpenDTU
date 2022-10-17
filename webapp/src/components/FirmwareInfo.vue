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
                            <td>{{ systemStatus.hostname }}</td>
                        </tr>
                        <tr>
                            <th>SDK Version</th>
                            <td>{{ systemStatus.sdkversion }}</td>
                        </tr>
                        <tr>
                            <th>Config Version</th>
                            <td>{{ systemStatus.config_version }}</td>
                        </tr>
                        <tr>
                            <th>Firmware Version / Git Hash</th>
                            <td><a :href="'https://github.com/tbnobody/OpenDTU/commits/' + systemStatus.git_hash?.substring(1)"
                                    target="_blank">{{ systemStatus.git_hash?.substring(1) }}</a></td>
                        </tr>
                        <tr>
                            <th>Firmware Update</th>
                            <td><a :href="systemStatus.update_url" target="_blank"><span class="badge"
                                        :class="systemStatus.update_status">{{
                                        systemStatus.update_text }}</span></a></td>
                        </tr>
                        <tr>
                            <th>Reset Reason CPU 0</th>
                            <td>{{ systemStatus.resetreason_0 }}</td>
                        </tr>
                        <tr>
                            <th>Reset Reason CPU 1</th>
                            <td>{{ systemStatus.resetreason_1 }}</td>
                        </tr>
                        <tr>
                            <th>Config save count</th>
                            <td>{{ systemStatus.cfgsavecount }}</td>
                        </tr>
                        <tr>
                            <th>Uptime</th>
                            <td>{{ timeInHours(systemStatus.uptime) }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, type PropType } from 'vue';
import type { SystemStatus } from '@/types/SystemStatus';

export default defineComponent({
    props: {
        systemStatus: { type: Object as PropType<SystemStatus>, required: true },
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
