<template>
    <div class="container" role="main">
        <div class="page-header">
            <h1>System Info</h1>
        </div>
        <FirmwareInfo />
        <div class="mt-5"></div>
        <HardwareInfo />
        <div class="mt-5"></div>
        <MemoryInfo />
        <div class="mt-5"></div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import HardwareInfo from "@/components/partials/HardwareInfo.vue";
import FirmwareInfo from "@/components/partials/FirmwareInfo.vue";
import MemoryInfo from "@/components/partials/MemoryInfo.vue";

export default defineComponent({
    components: {
        HardwareInfo,
        FirmwareInfo,
        MemoryInfo,
    },
});
</script>
