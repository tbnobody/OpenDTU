<template>
    <BasePage :title="$t('deviceadmin.DeviceManager')" :isLoading="dataLoading || pinMappingLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <nav>
            <div class="nav nav-tabs" id="nav-tab" role="tablist">
                <button class="nav-link active" id="nav-pin-tab" data-bs-toggle="tab" data-bs-target="#nav-pin"
                    type="button" role="tab" aria-controls="nav-pin" aria-selected="true">{{
                        $t('deviceadmin.PinAssignment')
                    }}</button>
            </div>
        </nav>
        <div class="tab-content" id="nav-tabContent">
            <div class="tab-pane fade show active" id="nav-pin" role="tabpanel" aria-labelledby="nav-pin-tab"
                tabindex="0">
                <div class="card">
                    <div class="card-body">

                        <form @submit="savePinConfig">
                            <div class="row mb-3">
                                <label for="inputPinProfile" class="col-sm-2 col-form-label">{{
                                    $t('deviceadmin.SelectedProfile')
                                }}</label>
                                <div class="col-sm-10">
                                    <select class="form-select" id="inputPinProfile"
                                        v-model="deviceConfigList.curPin.name">
                                        <option v-for="device in pinMappingList" :value="device.name" :key="device.name">
                                            {{ device.name }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <div class="alert alert-danger mt-3" role="alert" v-html="$t('deviceadmin.ProfileHint')">
                            </div>

                            <PinInfo
                                :selectedPinAssignment="pinMappingList.find(i => i.name === deviceConfigList.curPin.name)"
                                :currentPinAssignment="deviceConfigList.curPin" />

                            <button type="submit" class="btn btn-primary mb-3">{{ $t('deviceadmin.Save') }}</button>
                        </form>
                    </div>
                </div>
            </div>
        </div>


    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import PinInfo from '@/components/PinInfo.vue';
import type { DeviceConfig } from "@/types/DeviceConfig";
import type { PinMapping, Device } from "@/types/PinMapping";
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        PinInfo,
    },
    data() {
        return {
            dataLoading: true,
            pinMappingLoading: true,
            deviceConfigList: {} as DeviceConfig,
            pinMappingList: {} as PinMapping,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        }
    },
    created() {
        this.getDeviceConfig();
        this.getPinMappingList();
    },
    methods: {
        getPinMappingList() {
            this.pinMappingLoading = true;
            fetch("/api/config/get?file=pin_mapping.json", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (data) => {
                        this.pinMappingList = data;
                    }
                )
                .catch(() => {
                    this.pinMappingList = Array<Device>();
                })
                .finally(() => {
                    this.pinMappingList.push({
                        "name": this.$t('deviceadmin.DefaultProfile')
                    } as Device);
                    this.pinMappingList.sort((a, b) => (a.name < b.name) ? -1 : 1);
                    this.pinMappingLoading = false;
                });
        },
        getDeviceConfig() {
            this.dataLoading = true;
            fetch("/api/device/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (data) => {
                        this.deviceConfigList = data;
                        this.dataLoading = false;
                    }
                );
        },
        savePinConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.deviceConfigList));

            fetch("/api/device/config", {
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