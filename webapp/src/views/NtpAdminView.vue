<template>
    <BasePage :title="$t('ntpadmin.NtpSettings')" :isLoading="dataLoading || timezoneLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveNtpConfig">
            <div class="card">
                <div class="card-header text-bg-primary">{{ $t('ntpadmin.NtpConfiguration') }}</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputNtpServer" class="col-sm-2 col-form-label">
                            {{ $t('ntpadmin.TimeServer') }}
                            <BIconInfoCircle v-tooltip :title="$t('ntpadmin.TimeServerHint')" />
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputNtpServer" maxlength="32"
                                placeholder="Time Server" v-model="ntpConfigList.ntp_server" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputTimezone" class="col-sm-2 col-form-label">{{ $t('ntpadmin.Timezone') }}</label>
                        <div class="col-sm-10">
                            <select class="form-select" v-model="timezoneSelect">
                                <option v-for="(config, name) in timezoneList" :key="name + '---' + config"
                                    :value="name + '---' + config">
                                    {{ name }}
                                </option>
                            </select>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputTimezoneConfig" class="col-sm-2 col-form-label">
                            {{ $t('ntpadmin.TimezoneConfig') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputTimezoneConfig" maxlength="32"
                                placeholder="Timezone" v-model="ntpConfigList.ntp_timezone" disabled />
                        </div>
                    </div>
                </div>
            </div>
            <button type="submit" class="btn btn-primary mb-3">{{ $t('ntpadmin.Save') }}</button>
        </form>

        <div class="card">
            <div class="card-header text-bg-primary">{{ $t('ntpadmin.ManualTimeSynchronization') }}</div>
            <div class="card-body">
                <div class="row mb-3">
                    <label for="currentMcuTime" class="col-sm-2 col-form-label">
                        {{ $t('ntpadmin.CurrentOpenDtuTime') }}
                    </label>
                    <div class="col-sm-10">
                        <input type="text" class="form-control" id="currentMcuTime" v-model="mcuTime" disabled />
                    </div>
                </div>
                <div class="row mb-3">
                    <label for="currentLocalTime" class="col-sm-2 col-form-label">
                        {{ $t('ntpadmin.CurrentLocalTime') }}
                    </label>
                    <div class="col-sm-10">
                        <input type="text" class="form-control" id="currentLocalTime" v-model="localTime" disabled />
                    </div>
                </div>
                <div class="text-center mb-3">
                    <button type="button" class="btn btn-danger" @click="setCurrentTime()">
                        {{ $t('ntpadmin.SynchronizeTime') }}
                    </button>
                </div>
                <div class="alert alert-secondary" role="alert" v-html="$t('ntpadmin.SynchronizeTimeHint')"></div>

            </div>
        </div>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import type { NtpConfig } from "@/types/NtpConfig";
import {
    BIconInfoCircle,
} from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        BIconInfoCircle,
    },
    data() {
        return {
            dataLoading: true,
            timezoneLoading: true,
            ntpConfigList: {} as NtpConfig,
            timezoneList: {},
            timezoneSelect: "",
            mcuTime: new Date(),
            localTime: new Date(),
            dataAgeInterval: 0,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    watch: {
        timezoneSelect: function (newValue) {
            this.ntpConfigList.ntp_timezone = newValue.split("---")[1];
            this.ntpConfigList.ntp_timezone_descr = newValue.split("---")[0];
        },
    },
    created() {
        this.getTimezoneList();
        this.getNtpConfig();
        this.getCurrentTime();
        this.initDataAgeing();
    },
    methods: {
        initDataAgeing() {
            this.dataAgeInterval = setInterval(() => {
                this.mcuTime = new Date(this.mcuTime.setSeconds(this.mcuTime.getSeconds() + 1));
                this.localTime = new Date(this.localTime.setSeconds(this.localTime.getSeconds() + 1));
            }, 1000);
        },
        getTimezoneList() {
            this.timezoneLoading = true;
            fetch("/zones.json")
                .then((response) => response.json())
                .then((data) => {
                    this.timezoneList = data;
                    this.timezoneLoading = false;
                });
        },
        getNtpConfig() {
            this.dataLoading = true;
            fetch("/api/ntp/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (data) => {
                        this.ntpConfigList = data;
                        this.timezoneSelect =
                            this.ntpConfigList.ntp_timezone_descr +
                            "---" +
                            this.ntpConfigList.ntp_timezone;
                        this.dataLoading = false;
                    }
                );
        },
        getCurrentTime() {
            this.dataLoading = true;
            fetch("/api/ntp/time", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (data) => {
                        this.mcuTime = new Date(
                            data.year, data.month - 1, data.day,
                            data.hour, data.minute, data.second);
                        this.dataLoading = false;
                    }
                );
        },
        setCurrentTime() {
            const formData = new FormData();
            const time = {
                year: this.localTime.getFullYear(),
                month: this.localTime.getMonth() + 1,
                day: this.localTime.getDate(),
                hour: this.localTime.getHours(),
                minute: this.localTime.getMinutes(),
                second: this.localTime.getSeconds(),
            };
            console.log(time);
            formData.append("data", JSON.stringify(time));

            fetch("/api/ntp/time", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                )
                .then(() => {
                    this.getCurrentTime();
                });
        },
        saveNtpConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.ntpConfigList));

            fetch("/api/ntp/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>