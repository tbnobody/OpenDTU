<template>
    <NavBar />
    <main class="container-fluid">
        <router-view />
    </main>
</template>

<script lang="ts">
import NavBar from "./components/NavBar.vue";

export default {
    name: "App",
    components: {
        NavBar,
    },
};
</script>

<style>
/* Show it is fixed to the top */
body {
    min-height: 75rem;
    padding-top: 4.5rem;
}
</style>
