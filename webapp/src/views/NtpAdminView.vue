<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>NTP Settings</h1>
        </div>
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <div class="text-center" v-if="dataLoading || timezoneLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </div>

        <template v-if="!dataLoading && !timezoneLoading">
            <form @submit="saveNtpConfig">
                <div class="card">
                    <div class="card-header text-white bg-primary">NTP Configuration</div>
                    <div class="card-body">
                        <div class="row mb-3">
                            <label for="inputNtpServer" class="col-sm-2 col-form-label">Time Server:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputNtpServer" maxlength="32"
                                    placeholder="Time Server" v-model="ntpConfigList.ntp_server" />
                            </div>
                        </div>

                        <div class="row mb-3">
                            <label for="inputTimezone" class="col-sm-2 col-form-label">Timezone:</label>
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
                            <label for="inputTimezoneConfig" class="col-sm-2 col-form-label">Timezone Config:</label>
                            <div class="col-sm-10">
                                <input type="text" class="form-control" id="inputTimezoneConfig" maxlength="32"
                                    placeholder="Timezone" v-model="ntpConfigList.ntp_timezone" disabled />
                            </div>
                        </div>
                    </div>
                </div>
                <button type="submit" class="btn btn-primary mb-3">Save</button>
            </form>
        </template>

        <template v-if="!dataLoading && !timezoneLoading">
            <div class="card">
                <div class="card-header text-white bg-primary">Manual Time Synchronization</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="currentMcuTime" class="col-sm-2 col-form-label">Current OpenDTU Time:</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="currentMcuTime" v-model="mcuTime" disabled />
                        </div>
                    </div>
                    <div class="row mb-3">
                        <label for="currentLocalTime" class="col-sm-2 col-form-label">Current Local Time:</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="currentLocalTime" v-model="localTime"
                                disabled />
                        </div>
                    </div>
                    <div class="text-center mb-3">
                        <button type="button" class="btn btn-danger" @click="setCurrentTime()"
                            title="Synchronize Time">Synchronize Time
                        </button>
                    </div>
                    <div class="alert alert-secondary" role="alert">
                        <b>Hint:</b> You can use the manual time synchronization to set the current time of OpenDTU if
                        no NTP server is available. But be aware, that in case of power cycle the time gets lost. Also
                        the time accurancy can be very bad as it is not resynchronised regularly.
                    </div>

                </div>
            </div>
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";

export default defineComponent({
    components: {
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: true,
            timezoneLoading: true,
            ntpConfigList: {
                ntp_server: "",
                ntp_timezone: "",
                ntp_timezone_descr: ""
            },
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
            fetch("/api/ntp/config")
                .then((response) => response.json())
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
            fetch("/api/ntp/time")
                .then((response) => response.json())
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
                body: formData,
            })
                .then(function (response) {
                    if (response.status != 200) {
                        throw response.status;
                    } else {
                        return response.json();
                    }
                })
                .then(
                    (response) => {
                        this.alertMessage = response.message;
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
                body: formData,
            })
                .then(function (response) {
                    if (response.status != 200) {
                        throw response.status;
                    } else {
                        return response.json();
                    }
                })
                .then(
                    (response) => {
                        this.alertMessage = response.message;
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>