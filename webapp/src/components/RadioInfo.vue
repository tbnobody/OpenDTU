<template>
    <CardElement :text="$t('radioinfo.RadioInformation')" textVariant="text-bg-primary">
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('radioinfo.ChipStatus') }}</th>
                        <td>
                            <StatusBadge :status="systemStatus.radio_connected" true_text="radioinfo.Connected" false_text="radioinfo.NotConnected" />
                        </td>
                    </tr>
                    <tr>
                        <th>{{ $t('radioinfo.ChipType') }}</th>
                        <td>
                            <span class="badge" :class="{
                                'text-bg-danger': systemStatus.radio_connected && !systemStatus.radio_pvariant,
                                'text-bg-success': systemStatus.radio_connected && systemStatus.radio_pvariant,
                                'text-bg-secondary': !systemStatus.radio_connected,
                            }">
                                <template
                                    v-if="systemStatus.radio_connected && systemStatus.radio_pvariant">nRF24L01+</template>
                                <template
                                    v-else-if="systemStatus.radio_connected && !systemStatus.radio_pvariant">nRF24L01</template>
                                <template v-else>{{ $t('radioinfo.Unknown') }}</template>
                            </span>
                        </td>
                    </tr>
                </tbody>
            </table>
        </div>
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import StatusBadge from './StatusBadge.vue';
import type { SystemStatus } from '@/types/SystemStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
        StatusBadge,
    },
    props: {
        systemStatus: { type: Object as PropType<SystemStatus>, required: true },
    },
});
</script>
