<template>
    <div class="card">
        <div class="card-header text-bg-primary">
            {{ $t('hardwareinfo.HardwareInformation') }}
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('hardwareinfo.ChipModel') }}</th>
                            <td>{{ systemStatus.chipmodel }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('hardwareinfo.ChipRevision') }}</th>
                            <td>{{ systemStatus.chiprevision }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('hardwareinfo.ChipCores') }}</th>
                            <td>{{ systemStatus.chipcores }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('hardwareinfo.CpuFrequency') }}</th>
                            <td>{{ systemStatus.cpufreq }} {{ $t('hardwareinfo.Mhz') }}</td>
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
