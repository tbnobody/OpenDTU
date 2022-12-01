<template>
    <div class="card">
        <div class="card-header text-bg-primary">
            {{ $t('radioinfo.RadioInformation') }}
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('radioinfo.ChipStatus') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !systemStatus.radio_connected,
                                'text-bg-success': systemStatus.radio_connected,
                            }">
                                <span v-if="systemStatus.radio_connected">{{ $t('radioinfo.Connected') }}</span>
                                <span v-else>{{ $t('radioinfo.NotConnected') }}</span>
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('radioinfo.ChipType') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': systemStatus.radio_connected && !systemStatus.radio_pvariant,
                                'text-bg-success': systemStatus.radio_connected && systemStatus.radio_pvariant,
                                'text-bg-secondary': !systemStatus.radio_connected,
                            }">
                                <span
                                    v-if="systemStatus.radio_connected && systemStatus.radio_pvariant">nRF24L01+</span>
                                <span
                                    v-else-if="systemStatus.radio_connected && !systemStatus.radio_pvariant">nRF24L01</span>
                                <span v-else>{{ $t('radioinfo.Unknown') }}</span>
                            </td>
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
});
</script>
