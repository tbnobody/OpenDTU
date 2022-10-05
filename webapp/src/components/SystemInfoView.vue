<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>System Info</h1>
        </div>

        <div class="text-center" v-if="dataLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </div>

        <template v-if="!dataLoading">
            <FirmwareInfo v-bind="systemDataList" />
            <div class="mt-5"></div>
            <HardwareInfo v-bind="systemDataList" />
            <div class="mt-5"></div>
            <MemoryInfo v-bind="systemDataList" />
            <div class="mt-5"></div>
        </template>
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
    data() {
        return {
            dataLoading: true,
            systemDataList: {
                // HardwareInfo
                chipmodel: "",
                chiprevision: 0,
                chipcores: 0,
                cpufreq: 0,
                // FirmwareInfo
                hostname: "",
                sdkversion: "",
                config_version: "",
                git_hash: "",
                resetreason_0: "",
                resetreason_1: "",
                cfgsavecount: 0,
                uptime: 0,
                // MemoryInfo
                heap_total: 0,
                heap_used: 0,
                littlefs_total: 0,
                littlefs_used: 0,
                sketch_total: 0,
                sketch_used: 0
            }
        }
    },
    created() {
        this.getSystemInfo();
    },
    methods: {
        getSystemInfo() {
            this.dataLoading = true;
            fetch("/api/system/status")
                .then((response) => response.json())
                .then((data) => {
                    this.systemDataList = data;
                    this.dataLoading = false;
                })
        },
    },
});
</script>
