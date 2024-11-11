<template>
    <select class="form-select" @change="setLocale(($event.target as HTMLSelectElement).value)" :value="$i18n.locale">
        <option v-for="locale in allLocales" :key="`locale-${locale.code}`" :value="locale.code">
            {{ locale.name }}
        </option>
    </select>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { allLocales, setLocale } from '@/i18n';

export default defineComponent({
    name: 'LocaleSwitcher',
    data() {
        return {
            allLocales,
        };
    },
    methods: {
        setLocale,
    },
});
</script>
