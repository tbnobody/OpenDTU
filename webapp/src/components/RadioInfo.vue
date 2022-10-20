<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            Radio Information
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Chip Status</th>
                            <td class="badge" :class="{
                                'bg-danger': !systemStatus.radio_connected,
                                'bg-success': systemStatus.radio_connected,
                            }">
                                <span v-if="systemStatus.radio_connected">connected</span>
                                <span v-else>not connected</span>
                            </td>
                        </tr>
                        <tr>
                            <th>Chip Type</th>
                            <td class="badge" :class="{
                                'bg-danger': systemStatus.radio_connected && !systemStatus.radio_pvariant,
                                'bg-success': systemStatus.radio_connected && systemStatus.radio_pvariant,
                                'bg-secondary': !systemStatus.radio_connected,
                            }">
                                <span
                                    v-if="systemStatus.radio_connected && systemStatus.radio_pvariant">nRF24L01+</span>
                                <span
                                    v-else-if="systemStatus.radio_connected && !systemStatus.radio_pvariant">nRF24L01</span>
                                <span v-else>Unknown</span>
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
