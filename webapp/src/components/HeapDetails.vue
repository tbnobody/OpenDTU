<template>
    <CardElement :text="$t('heapdetails.HeapDetails')" textVariant="text-bg-primary">
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('heapdetails.TotalFree') }}</th>
                        <td>{{ $n(Math.round(getFreeHeap() / 1024), 'kilobyte') }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('heapdetails.LargestFreeBlock') }}</th>
                        <td>
                            {{ $n(Math.round(systemStatus.heap_max_block / 1024), 'kilobyte') }}
                        </td>
                    </tr>
                    <tr>
                        <th>{{ $t('heapdetails.Fragmentation') }}</th>
                        <td>{{ $n(getFragmentation(), 'percent') }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('heapdetails.MaxUsage') }}</th>
                        <td>
                            {{ $n(Math.round(getMaxUsageAbs() / 1024), 'kilobyte') }} ({{
                                $n(getMaxUsageRel(), 'percent')
                            }})
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
    methods: {
        getFreeHeap() {
            return this.systemStatus.heap_total - this.systemStatus.heap_used;
        },
        getMaxUsageAbs() {
            return this.systemStatus.heap_total - this.systemStatus.heap_min_free;
        },
        getMaxUsageRel() {
            return this.getMaxUsageAbs() / this.systemStatus.heap_total;
        },
        getFragmentation() {
            return 1 - this.systemStatus.heap_max_block / this.getFreeHeap();
        },
    },
});
</script>
