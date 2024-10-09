<template>
    <div :class="['card', addSpace ? 'mt-5' : '']">
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
        addSpace: Boolean,
        centerContent: Boolean,
    },
});
</script>
