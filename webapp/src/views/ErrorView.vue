<template>
    <BasePage :title="$t('Error.Oops')">
        <div class="alert alert-danger" role="alert">
            <h2>{{ $route.params.message }}</h2>
        </div>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
    },
});
</script>
