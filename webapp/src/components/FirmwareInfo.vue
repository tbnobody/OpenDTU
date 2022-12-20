<template>
    <div class="card">
        <div class="card-header text-bg-primary">
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
                                    target="_blank" v-tooltip title="Click here to show information about your current version">{{
                                            systemStatus.git_hash?.substring(1)
                                    }}</a></td>
                        </tr>
                        <tr>
                            <th>Firmware Update</th>
                            <td><a :href="systemStatus.update_url" target="_blank" v-tooltip title="Click here to view the changes between your version 
                                    and the latest version"><span class="badge" :class="systemStatus.update_status">{{
                                            systemStatus.update_text
                                    }}</span></a></td>
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
import { timestampToString } from '@/utils';

export default defineComponent({
    props: {
        systemStatus: { type: Object as PropType<SystemStatus>, required: true },
    },
    computed: {
        timeInHours() {
            return (value: number) => {
                return timestampToString(value, true);
            };
        },
    },
});
</script>
