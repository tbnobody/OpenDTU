<template>
    <div class="card" :class="{ 'border-info': channelNumber == 0 }">
        <div v-if="channelNumber >= 1" class="card-header">
            <template v-if="channelData.name.u != ''">{{ channelData.name.u }}</template>
            <template v-else>{{ $t('inverterchannelinfo.String', { num: channelNumber }) }}</template>
        </div>
        <div v-if="channelNumber == 0" class="card-header text-bg-info">
            {{ $t('inverterchannelinfo.Phase', { num: channelNumber + 1 }) }}</div>
        <div class="card-body">
            <table class="table table-striped table-hover">
                <thead>
                    <tr>
                        <th scope="col">{{ $t('inverterchannelinfo.Property') }}</th>
                        <th style="text-align: right" scope="col">{{ $t('inverterchannelinfo.Value') }}</th>
                        <th scope="col">{{ $t('inverterchannelinfo.Unit') }}</th>
                    </tr>
                </thead>
                <tbody>
                    <tr v-for="(property, key) in channelData" :key="`prop-${key}`">
                        <template v-if="key != 'name' && property">
                            <th scope="row">{{ $t('inverterchannelproperty.' + key) }}</th>
                            <td style="text-align: right">{{ formatNumber(property.v, property.d) }}</td>
                            <td>{{ property.u }}</td>
                        </template>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, type PropType } from 'vue';
import type { InverterStatistics } from '@/types/LiveDataStatus';
import { formatNumber } from '@/utils';

export default defineComponent({
    props: {
        channelData: { type: Object as PropType<InverterStatistics>, required: true },
        channelNumber: { type: Number, required: true },
    },
    methods: {
        formatNumber,
    },
});
</script>
