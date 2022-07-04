<template>
    <div class="card">
        <div v-if="channelNumber >= 1" class="card-header">String {{ channelNumber }}</div>
        <div v-if="channelNumber == 0" class="card-header">Phase {{ channelNumber + 1 }}</div>
        <div class="card-body">
            <table class="table table-striped table-hover">
                <thead>
                    <tr>
                        <th scope="col">Property</th>
                        <th scope="col">Value</th>
                        <th scope="col">Unit</th>
                    </tr>
                </thead>
                <tbody>
                    <tr v-for="(property, key) in channelData" :key="`prop-${key}`">
                        <th scope="row">{{ key }}</th>
                        <td style="text-align: right">{{ formatNumber(property.v) }}</td>
                        <td>{{ property.u }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    props: {
        channelData: Object,
        channelNumber: { type: Number, required: true },
    },
    methods: {
        formatNumber(num: string) {
            return parseFloat(num).toFixed(2);
        },
    },
});
</script>
