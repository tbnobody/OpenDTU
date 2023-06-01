<template>
    <BasePage :title="$t('deviceadmin.DeviceManager')" :isLoading="dataLoading || pinMappingLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="savePinConfig">
            <nav>
                <div class="nav nav-tabs" id="nav-tab" role="tablist">
                    <button class="nav-link active" id="nav-pin-tab" data-bs-toggle="tab" data-bs-target="#nav-pin"
                        type="button" role="tab" aria-controls="nav-pin" aria-selected="true">{{
                            $t('deviceadmin.PinAssignment')
                        }}</button>
                    <button class="nav-link" id="nav-display-tab" data-bs-toggle="tab" data-bs-target="#nav-display"
                        type="button" role="tab" aria-controls="nav-display">{{ $t('deviceadmin.Display') }}</button>
                </div>
            </nav>
            <div class="tab-content" id="nav-tabContent">
                <div class="tab-pane fade show active" id="nav-pin" role="tabpanel" aria-labelledby="nav-pin-tab"
                    tabindex="0">
                    <div class="card">
                        <div class="card-body">
                            <div class="row mb-3">
                                <label for="inputPinProfile" class="col-sm-2 col-form-label">{{
                                    $t('deviceadmin.SelectedProfile')
                                }}</label>
                                <div class="col-sm-10">
                                    <select class="form-select" id="inputPinProfile"
                                        v-model="deviceConfigList.curPin.name">
                                        <option v-for="device in pinMappingList" :value="device.name" :key="device.name">
                                            {{ device.name === "Default" ? $t('deviceadmin.DefaultProfile') : device.name }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <div class="alert alert-danger mt-3" role="alert" v-html="$t('deviceadmin.ProfileHint')">
                            </div>

                            <PinInfo
                                :selectedPinAssignment="pinMappingList.find(i => i.name === deviceConfigList.curPin.name)"
                                :currentPinAssignment="deviceConfigList.curPin" />
                        </div>
                    </div>
                </div>

                <div class="tab-pane fade show" id="nav-display" role="tabpanel" aria-labelledby="nav-display-tab"
                    tabindex="0">
                    <div class="card">
                        <div class="card-body">
                            <InputElement :label="$t('deviceadmin.PowerSafe')"
                                v-model="deviceConfigList.display.power_safe" type="checkbox"
                                :tooltip="$t('deviceadmin.PowerSafeHint')" />

                            <InputElement :label="$t('deviceadmin.Screensaver')"
                                v-model="deviceConfigList.display.screensaver" type="checkbox"
                                :tooltip="$t('deviceadmin.ScreensaverHint')" />

                            <div class="row mb-3">
                                <label class="col-sm-2 col-form-label">
                                    {{ $t('deviceadmin.DisplayLanguage') }}
                                </label>
                                <div class="col-sm-10">
                                    <select class="form-select" v-model="deviceConfigList.display.language">
                                        <option v-for="language in displayLanguageList" :key="language.key" :value="language.key">
                                            {{ $t(`deviceadmin.` + language.value) }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <div class="row mb-3">
                                <label class="col-sm-2 col-form-label">
                                    {{ $t('deviceadmin.Rotation') }}
                                </label>
                                <div class="col-sm-10">
                                    <select class="form-select" v-model="deviceConfigList.display.rotation">
                                        <option v-for="rotation in displayRotationList" :key="rotation.key" :value="rotation.key">
                                            {{ $t(`deviceadmin.` + rotation.value) }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <div class="row mb-3">
                                <label for="inputDisplayContrast" class="col-sm-2 col-form-label">{{
                                    $t('deviceadmin.Contrast', { contrast: $n(deviceConfigList.display.contrast / 100,
                                        'percent')
                                }) }}</label>
                                <div class="col-sm-10">
                                    <input type="range" class="form-range" min="0" max="100" id="inputDisplayContrast"
                                        v-model="deviceConfigList.display.contrast" />
                                </div>
                            </div>

                        </div>
                    </div>
                </div>
            </div>

            <button type="submit" class="btn btn-primary mb-3">{{ $t('deviceadmin.Save') }}</button>

        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import InputElement from '@/components/InputElement.vue';
import PinInfo from '@/components/PinInfo.vue';
import type { DeviceConfig } from "@/types/DeviceConfig";
import type { PinMapping, Device } from "@/types/PinMapping";
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        InputElement,
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
            displayRotationList: [
                { key: 0, value: 'rot0' },
                { key: 1, value: 'rot90' },
                { key: 2, value: 'rot180' },
                { key: 3, value: 'rot270' },
            ],
            displayLanguageList: [
                { key: 0, value: "en" },
                { key: 1, value: "de" },
                { key: 2, value: "fr" },
            ],
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
                    this.pinMappingList.sort((a, b) => (a.name < b.name) ? -1 : 1);
                    this.pinMappingList.splice(0, 0, { "name": "Default" } as Device);
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