<template>
    <span class="badge" :class="[status ? true_class : false_class]">
        <template v-if="status">{{ $t(true_text) }}</template>
        <template v-else>{{ $t(false_text) }}</template>
    </span>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    props: {
        'status': Boolean,
        'true_text': {
            type: String,
            required: true
        },
        'false_text': {
            type: String,
            required: true
        },
        'true_class': {
            type: String,
            default: 'text-bg-success'
        },
        'false_class': {
            type: String,
            default: 'text-bg-danger'
        }
    },
});
</script>