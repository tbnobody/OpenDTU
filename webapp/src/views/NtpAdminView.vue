<template>
    <BasePage :title="$t('ntpadmin.NtpSettings')" :isLoading="dataLoading || timezoneLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveNtpConfig">
            <CardElement :text="$t('ntpadmin.NtpConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('ntpadmin.TimeServer')"
                              v-model="ntpConfigList.ntp_server"
                              type="text" maxlength="32"
                              :tooltip="$t('ntpadmin.TimeServerHint')"/>

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

                <InputElement :label="$t('ntpadmin.TimezoneConfig')"
                              v-model="ntpConfigList.ntp_timezone"
                              type="text" maxlength="32" disabled/>
            </CardElement>

            <CardElement :text="$t('ntpadmin.LocationConfiguration')" textVariant="text-bg-primary" add-space>
                <InputElement :label="$t('ntpadmin.Latitude')"
                              v-model="ntpConfigList.latitude"
                              type="number" min="-90" max="90" step="any"/>

                <InputElement :label="$t('ntpadmin.Longitude')"
                              v-model="ntpConfigList.longitude"
                              type="number" min="-180" max="180" step="any"/>


                <div class="row mb-3">
                    <label class="col-sm-2 col-form-label">
                        {{ $t('ntpadmin.SunSetType') }}
                        <BIconInfoCircle v-tooltip :title="$t('ntpadmin.SunSetTypeHint')" />
                    </label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="ntpConfigList.sunsettype">
                            <option v-for="sunsettype in sunsetTypeList" :key="sunsettype.key" :value="sunsettype.key">
                                {{ $t(`ntpadmin.` + sunsettype.value) }}
                            </option>
                        </select>
                    </div>
                </div>
            </CardElement>
            <button type="submit" class="btn btn-primary mb-3">{{ $t('ntpadmin.Save') }}</button>
        </form>

        <CardElement :text="$t('ntpadmin.ManualTimeSynchronization')" textVariant="text-bg-primary" add-space>
            <InputElement :label="$t('ntpadmin.CurrentOpenDtuTime')"
                           v-model="mcuTime"
                           type="text" disabled/>

            <InputElement :label="$t('ntpadmin.CurrentLocalTime')"
                          v-model="localTime"
                          type="text" disabled/>

            <div class="text-center mb-3">
                <button type="button" class="btn btn-danger" @click="setCurrentTime()">
                    {{ $t('ntpadmin.SynchronizeTime') }}
                </button>
            </div>
            <div class="alert alert-secondary" role="alert" v-html="$t('ntpadmin.SynchronizeTimeHint')"></div>
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import type { NtpConfig } from "@/types/NtpConfig";
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';
import { BIconInfoCircle } from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        InputElement,
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
            sunsetTypeList: [
                { key: 0, value: 'OFFICIAL' },
                { key: 1, value: 'NAUTICAL' },
                { key: 2, value: 'CIVIL' },
                { key: 3, value: 'ASTONOMICAL' },
            ],
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