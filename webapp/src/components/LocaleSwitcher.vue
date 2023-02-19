<template>
    <select class="form-select" @change="updateLanguage()" v-model="$i18n.locale">
        <option v-for="locale in $i18n.availableLocales" :key="`locale-${locale}`" :value="locale">
            {{ getLocaleName(locale) }}
        </option>
    </select>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { LOCALES } from '@/locales';

export default defineComponent({
    name: "LocaleSwitcher",
    methods: {
        updateLanguage() {
            localStorage.setItem("locale", this.$i18n.locale);
        },
        getLocaleName(locale: string): string {
            return LOCALES.find(i => i.value === locale)?.caption || "";
        }
    },
    mounted() {
        if (localStorage.getItem("locale")) {
            this.$i18n.locale = localStorage.getItem("locale") || "en";
        } else {
            localStorage.setItem("locale", this.$i18n.locale);
        }
    },
});
</script>