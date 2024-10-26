<template>
    <div :class="['card', table ? 'card-table' : '', addSpace ? 'mt-5' : '']">
        <div :class="['card-header', textVariant]">{{ text }}</div>
        <div :class="['card-body', 'card-text', centerContent ? 'text-center' : '']">
            <slot />
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    props: {
        text: String,
        textVariant: String,
        table: Boolean,
        addSpace: Boolean,
        centerContent: Boolean,
    },
});
</script>
