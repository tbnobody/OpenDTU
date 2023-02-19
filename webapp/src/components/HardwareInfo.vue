<template>
    <CardElement :text="$t('hardwareinfo.HardwareInformation')" textVariant="text-bg-primary">
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
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import type { SystemStatus } from '@/types/SystemStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        systemStatus: { type: Object as PropType<SystemStatus>, required: true },
    },
});
</script>
