<template>
    <div style="padding-right: 2em">
        {{ $t('dataagedisplay.DataAge') }}:
        {{ $t('dataagedisplay.SecondsSince', { n: dataAgeSeconds }) }}
        <template v-if="dataAgeMs > thresholdMs"> ({{ calculateAbsoluteTime(dataAgeMs) }}) </template>
    </div>
</template>

<script lang="ts">
export default {
    name: 'DataAgeDisplay',
    props: {
        dataAgeMs: {
            type: Number,
            required: true,
        },
        thresholdMs: {
            type: Number,
            default: 300000,
        },
    },
    methods: {
        calculateAbsoluteTime(lastTime: number): string {
            const date = new Date(Date.now() - lastTime);
            return this.$d(date, 'datetime');
        },
    },
    computed: {
        dataAgeSeconds() {
            return Math.floor(this.dataAgeMs / 1000);
        },
    },
};
</script>
