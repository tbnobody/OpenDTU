<template>
    <CardElement :text="$t('firmwareinfo.FirmwareInformation')" textVariant="text-bg-primary">
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('firmwareinfo.Hostname') }}</th>
                        <td>{{ systemStatus.hostname }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('firmwareinfo.SdkVersion') }}</th>
                        <td>{{ systemStatus.sdkversion }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('firmwareinfo.ConfigVersion') }}</th>
                        <td>{{ systemStatus.config_version }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('firmwareinfo.FirmwareVersion') }}</th>
                        <td><a :href="versionInfoUrl"
                                target="_blank" v-tooltip :title="$t('firmwareinfo.FirmwareVersionHint')">
                                {{ systemStatus.git_hash }}
                            </a></td>
                    </tr>
                    <tr>
                        <th>{{ $t('firmwareinfo.FirmwareUpdate') }}</th>
                        <td><a :href="systemStatus.update_url" target="_blank" v-tooltip
                                :title="$t('firmwareinfo.FirmwareUpdateHint')">
                                <span class="badge" :class="systemStatus.update_status">
                                    {{ systemStatus.update_text }}
                                </span>
                            </a></td>
                    </tr>
                    <tr>
                        <th>{{ $t('firmwareinfo.ResetReason0') }}</th>
                        <td>{{ systemStatus.resetreason_0 }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('firmwareinfo.ResetReason1') }}</th>
                        <td>{{ systemStatus.resetreason_1 }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('firmwareinfo.ConfigSaveCount') }}</th>
                        <td>{{ $n(systemStatus.cfgsavecount, 'decimal') }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('firmwareinfo.Uptime') }}</th>
                        <td>{{ timeInHours(systemStatus.uptime) }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import type { SystemStatus } from '@/types/SystemStatus';
import { timestampToString } from '@/utils';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        systemStatus: { type: Object as PropType<SystemStatus>, required: true },
    },
    computed: {
        timeInHours() {
            return (value: number) => {
                return timestampToString(value, true);
            };
        },
        versionInfoUrl(): string {
            if (this.systemStatus.git_is_hash) {
                return 'https://github.com/tbnobody/OpenDTU/commits/' + this.systemStatus.git_hash;
            }
            return 'https://github.com/tbnobody/OpenDTU/releases/tag/' + this.systemStatus.git_hash;
        }
    },
});
</script>
