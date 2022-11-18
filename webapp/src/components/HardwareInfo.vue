<template>
    <div class="card">
        <div class="card-header text-bg-primary">
            Hardware Information
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Chip Model</th>
                            <td>{{ systemStatus.chipmodel }}</td>
                        </tr>
                        <tr>
                            <th>Chip Revision</th>
                            <td>{{ systemStatus.chiprevision }}</td>
                        </tr>
                        <tr>
                            <th>Chip Cores</th>
                            <td>{{ systemStatus.chipcores }}</td>
                        </tr>
                        <tr>
                            <th>CPU Frequency</th>
                            <td>{{ systemStatus.cpufreq }} MHz</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import type { SystemStatus } from '@/types/SystemStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    props: {
        systemStatus: { type: Object as PropType<SystemStatus>, required: true },
    },
});
</script>
