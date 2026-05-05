<template>
    <CardElement :text="title" textVariant="text-bg-primary" table>
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('memorydetails.TotalFree') }}</th>
                        <td>{{ $n(Math.round(getFreeMemory() / 1024), 'kilobyte') }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('memorydetails.LargestFreeBlock') }}</th>
                        <td>{{ $n(Math.round(maxBlock / 1024), 'kilobyte') }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('memorydetails.Fragmentation') }}</th>
                        <td>{{ $n(getFragmentation(), 'percent') }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('memorydetails.MaxUsage') }}</th>
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
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        title: { type: String, required: true },
        total: { type: Number, required: true },
        used: { type: Number, required: true },
        minFree: { type: Number, required: true },
        maxBlock: { type: Number, required: true },
    },
    methods: {
        getFreeMemory() {
            return this.total - this.used;
        },
        getMaxUsageAbs() {
            return this.total - this.minFree;
        },
        getMaxUsageRel() {
            return this.getMaxUsageAbs() / this.total;
        },
        getFragmentation() {
            return 1 - this.maxBlock / this.getFreeMemory();
        },
    },
});
</script>
