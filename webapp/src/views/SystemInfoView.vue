<template>
    <BasePage :title="$t('systeminfo.SystemInfo')" :isLoading="dataLoading">
        <FirmwareInfo :systemStatus="systemDataList" />
        <div class="mt-5"></div>
        <HardwareInfo :systemStatus="systemDataList" />
        <div class="mt-5"></div>
        <MemoryInfo :systemStatus="systemDataList" />
        <div class="mt-5"></div>
        <RadioInfo :systemStatus="systemDataList" />
        <div class="mt-5"></div>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { handleResponse, authHeader } from '@/utils/authentication';
import BasePage from '@/components/BasePage.vue';
import HardwareInfo from "@/components/HardwareInfo.vue";
import FirmwareInfo from "@/components/FirmwareInfo.vue";
import MemoryInfo from "@/components/MemoryInfo.vue";
import RadioInfo from "@/components/RadioInfo.vue";
import type { SystemStatus } from '@/types/SystemStatus';

export default defineComponent({
    components: {
        BasePage,
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
            fetch("/api/system/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
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
                    throw new Error(this.$t("systeminfo.VersionError"));
                })
                .then((data) => {
                    if (data.total_commits > 0) {
                        this.systemDataList.update_text = this.$t("systeminfo.VersionNew");
                        this.systemDataList.update_status = "text-bg-danger";
                        this.systemDataList.update_url = data.html_url;
                    } else {
                        this.systemDataList.update_text = this.$t("systeminfo.VersionOk");
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
