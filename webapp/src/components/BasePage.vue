<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>{{ title }}</h1>
        </div>

        <div class="text-center" v-if="isLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </div>

        <template v-if="!isLoading">
            <slot />
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    props: {
        title: { type: String, required: true },
        isLoading: { type: Boolean, required: false, default: false }
    },
});
</script>