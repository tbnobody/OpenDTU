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
            <FirmwareInfo :systemStatus="systemDataList" />
            <div class="mt-5"></div>
            <HardwareInfo :systemStatus="systemDataList" />
            <div class="mt-5"></div>
            <MemoryInfo :systemStatus="systemDataList" />
            <div class="mt-5"></div>
            <RadioInfo :systemStatus="systemDataList" />
            <div class="mt-5"></div>
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import HardwareInfo from "@/components/HardwareInfo.vue";
import FirmwareInfo from "@/components/FirmwareInfo.vue";
import MemoryInfo from "@/components/MemoryInfo.vue";
import RadioInfo from "@/components/RadioInfo.vue";
import type { SystemStatus } from '@/types/SystemStatus';

export default defineComponent({
    components: {
        HardwareInfo,
        FirmwareInfo,
        MemoryInfo,
        RadioInfo,
    },
    data() {
        return {
            dataLoading: true,
            systemDataList: {} as SystemStatus,
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
                    this.getUpdateInfo();
                })
        },
        getUpdateInfo() {
            const fetchUrl = "https://api.github.com/repos/tbnobody/OpenDTU/compare/"
                + this.systemDataList.git_hash?.substring(1) + "...HEAD";

            fetch(fetchUrl)
                .then((response) => {
                    if (response.ok) {
                        return response.json()
                    }
                    throw new Error('Error fetching version information');
                })
                .then((data) => {
                    if (data.total_commits > 0) {
                        this.systemDataList.update_text = "New version available! Show changes!"
                        this.systemDataList.update_status = "text-bg-danger";
                        this.systemDataList.update_url = data.html_url;
                    } else {
                        this.systemDataList.update_text = "Up to date!"
                        this.systemDataList.update_status = "text-bg-success";
                    }
                })
                .catch((error: Error) => {
                    this.systemDataList.update_text = error.message;
                    this.systemDataList.update_status = "text-bg-secondary";
                });
        }
    },
});
</script>
