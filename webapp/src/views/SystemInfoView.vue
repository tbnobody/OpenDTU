<template>
    <BasePage :title="$t('systeminfo.SystemInfo')" :isLoading="dataLoading" :show-reload="true" @reload="getSystemInfo">
        <FirmwareInfo :systemStatus="systemDataList" v-model:allowVersionInfo="allowVersionInfo" />
        <div class="mt-5"></div>
        <HardwareInfo :systemStatus="systemDataList" />
        <div class="mt-5"></div>
        <MemoryInfo :systemStatus="systemDataList" />
        <div class="mt-5"></div>
        <HeapDetails :systemStatus="systemDataList" />
        <div class="mt-5"></div>
        <TaskDetails :taskDetails="systemDataList.task_details" />
        <div class="mt-5"></div>
        <RadioInfo :systemStatus="systemDataList" />
        <div class="mt-5"></div>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import FirmwareInfo from '@/components/FirmwareInfo.vue';
import HardwareInfo from '@/components/HardwareInfo.vue';
import MemoryInfo from '@/components/MemoryInfo.vue';
import HeapDetails from '@/components/HeapDetails.vue';
import TaskDetails from '@/components/TaskDetails.vue';
import RadioInfo from '@/components/RadioInfo.vue';
import type { SystemStatus } from '@/types/SystemStatus';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        FirmwareInfo,
        HardwareInfo,
        MemoryInfo,
        HeapDetails,
        TaskDetails,
        RadioInfo,
    },
    data() {
        return {
            dataLoading: true,
            systemDataList: {} as SystemStatus,
            allowVersionInfo: false,
        };
    },
    created() {
        this.allowVersionInfo = (localStorage.getItem('allowVersionInfo') || '0') == '1';
        this.getSystemInfo();
    },
    methods: {
        getSystemInfo() {
            this.dataLoading = true;
            fetch('/api/system/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.systemDataList = data;
                    this.dataLoading = false;
                    this.getUpdateInfo();
                });
        },
        getUpdateInfo() {
            if (this.systemDataList.git_hash === undefined) {
                return;
            }

            // If the left char is a "g" the value is the git hash (remove the "g")
            this.systemDataList.git_is_hash = this.systemDataList.git_hash?.substring(0, 1) == 'g';
            this.systemDataList.git_hash = this.systemDataList.git_is_hash
                ? this.systemDataList.git_hash?.substring(1)
                : this.systemDataList.git_hash;

            // Handle format "v0.1-5-gabcdefh"
            if (this.systemDataList.git_hash?.lastIndexOf('-') >= 0) {
                this.systemDataList.git_hash = this.systemDataList.git_hash.substring(
                    this.systemDataList.git_hash.lastIndexOf('-') + 2
                );
                this.systemDataList.git_is_hash = true;
            }

            if (!this.allowVersionInfo) {
                return;
            }

            const fetchUrl =
                'https://api.github.com/repos/tbnobody/OpenDTU/compare/' +
                this.systemDataList.git_hash +
                '...' +
                this.systemDataList.git_branch;

            fetch(fetchUrl)
                .then((response) => {
                    if (response.ok) {
                        return response.json();
                    }
                    throw new Error(this.$t('systeminfo.VersionError'));
                })
                .then((data) => {
                    if (data.total_commits > 0) {
                        this.systemDataList.update_text = this.$t('systeminfo.VersionNew');
                        this.systemDataList.update_status = 'text-bg-danger';
                        this.systemDataList.update_url = data.html_url;
                    } else {
                        this.systemDataList.update_text = this.$t('systeminfo.VersionOk');
                        this.systemDataList.update_status = 'text-bg-success';
                    }
                })
                .catch((error: Error) => {
                    this.systemDataList.update_text = error.message;
                    this.systemDataList.update_status = 'text-bg-secondary';
                });
        },
    },
    watch: {
        allowVersionInfo(allow: boolean) {
            localStorage.setItem('allowVersionInfo', allow ? '1' : '0');
            if (allow) {
                this.getUpdateInfo();
            }
        },
    },
});
</script>
