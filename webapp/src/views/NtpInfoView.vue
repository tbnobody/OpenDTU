<template>
    <BasePage :title="'NTP Info'" :isLoading="dataLoading">
        <div class="card">
            <div class="card-header text-bg-primary">Configuration Summary</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Server</th>
                                <td>{{ ntpDataList.ntp_server }}</td>
                            </tr>
                            <tr>
                                <th>Timezone</th>
                                <td>{{ ntpDataList.ntp_timezone }}</td>
                            </tr>
                            <tr>
                                <th>Timezone Description</th>
                                <td>{{ ntpDataList.ntp_timezone_descr }}</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>

        <div class="card mt-5">
            <div class="card-header text-bg-primary">Current Time</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Status</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !ntpDataList.ntp_status,
                                    'text-bg-success': ntpDataList.ntp_status,
                                }">
                                    <span v-if="ntpDataList.ntp_status">synced</span>
                                    <span v-else>not synced</span>
                                </td>
                            </tr>
                            <tr>
                                <th>Local Time</th>
                                <td>{{ ntpDataList.ntp_localtime }}</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { handleResponse, authHeader } from '@/utils/authentication';
import BasePage from '@/components/BasePage.vue';
import type { NtpStatus } from "@/types/NtpStatus";

export default defineComponent({
    components: {
        BasePage,
    },
    data() {
        return {
            dataLoading: true,
            ntpDataList: {} as NtpStatus,
        };
    },
    created() {
        this.getNtpInfo();
    },
    methods: {
        getNtpInfo() {
            this.dataLoading = true;
            fetch("/api/ntp/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.ntpDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>