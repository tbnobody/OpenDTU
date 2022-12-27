<template>
    <CardElement :text="$t('radioinfo.RadioInformation')" textVariant="text-bg-primary">
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
