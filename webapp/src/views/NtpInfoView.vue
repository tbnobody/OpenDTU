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

        <div class="card mt-5">
            <div class="card-header text-bg-primary">Sunrise Sunset Information</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Status</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !ntpDataList.sunset_enabled,
                                    'text-bg-success': ntpDataList.sunset_enabled,
                                }">
                                    <span v-if="ntpDataList.sunset_enabled">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                            <tr>
                                <th>Timezone offset</th>
                                <td>{{ ntpDataList.timezone_offset }}</td>
                            </tr>
                            <tr>
                                <th>Sunrise time</th>
                                <td>{{ ntpDataList.sunrise_time }}</td>
                            </tr>
                            <tr>
                                <th>Sunset time</th>
                                <td>{{ ntpDataList.sunset_time }}</td>
                            </tr>
                            <tr>
                                <th>Status</th>
                                <td class="badge" :class="{
                                    'text-bg-warning': !ntpDataList.sunset_isdaytime,
                                    'text-bg-success': ntpDataList.sunset_isdaytime,
                                }">
                                    <span v-if="ntpDataList.sunset_isdaytime">Daytime</span>
                                    <span v-else>Nighttime</span>
                                </td>
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
            fetch("/api/ntp/status")
                .then((response) => response.json())
                .then((data) => {
                    this.ntpDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>