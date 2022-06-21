<template>
    <div class="container" role="main">
        <div class="page-header">
            <h1>NTP Settings</h1>
        </div>
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ this.alertMessage }}
        </BootstrapAlert>
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
    </div>
</template>

<script>
import { defineComponent } from 'vue';
import BootstrapAlert from "@/components/partials/BootstrapAlert.vue";

export default defineComponent({
    components: {
        BootstrapAlert,
    },
    data() {
        return {
            ntpConfigList: [],
            timezoneList: {},
            timezoneSelect: "",
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
    },
    methods: {
        getTimezoneList() {
            fetch("/zones.json")
                .then((response) => response.json())
                .then((data) => (this.timezoneList = data));
        },
        getNtpConfig() {
            fetch("/api/ntp/config")
                .then((response) => response.json())
                .then(
                    function (data) {
                        this.ntpConfigList = data;
                        this.timezoneSelect =
                            this.ntpConfigList.ntp_timezone_descr +
                            "---" +
                            this.ntpConfigList.ntp_timezone;
                    }.bind(this)
                );
        },
        saveNtpConfig(e) {
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
                    function (response) {
                        this.alertMessage = response.message;
                        this.alertType = response.type;
                        this.showAlert = true;
                    }.bind(this)
                );
        },
    },
});
</script>