<template>
    <select class="form-select" @change="updateLanguage()" v-model="$i18n.locale">
        <option v-for="locale in $i18n.availableLocales" :key="`locale-${locale}`" :value="locale">
            {{ locale.toUpperCase() }}
        </option>
    </select>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    name: "LocaleSwitcher",
    methods: {
        updateLanguage() {
            localStorage.setItem("locale", this.$i18n.locale);
        },
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