<template>
    <div class="text-center" v-if="dataLoading">
        <div class="spinner-border" role="status">
            <span class="visually-hidden">Loading...</span>
        </div>
    </div>

    <template v-else>
        <div class="row gy-3 mt-0">
            <div class="tab-content col-sm-12 col-md-12" id="v-pills-tabContent">
                <div class="card">
                    <div
                        class="card-header d-flex justify-content-between align-items-center"
                        :class="getStatusClass()"
                    >
                        <div class="p-1 flex-grow-1">
                            <div class="d-flex flex-wrap">
                                <div style="padding-right: 2em">
                                    <template v-if="data.vendorName !== undefined && data.productName !== undefined">
                                        {{ data.vendorName }} {{ data.productName }}
                                    </template>
                                    <template v-else>Grid Charger</template>
                                </div>
                                <div v-if="data.serial" style="padding-right: 2em">
                                    {{ $t('gridcharger.SerialNumber') }}: {{ data.serial }}
                                </div>
                                <DataAgeDisplay :data-age-ms="data.dataAge" />
                            </div>
                        </div>
                        <div class="btn-toolbar p-2" role="toolbar" v-if="data.provider === 0">
                            <div class="btn-group me-2" role="group">
                                <button
                                    :disabled="!data.reachable"
                                    type="button"
                                    class="btn btn-sm btn-danger"
                                    @click="onShowLimitSettings()"
                                    v-tooltip
                                    :title="$t('gridcharger.LimitSettings')"
                                >
                                    <BIconSpeedometer style="font-size: 24px" />
                                </button>
                            </div>
                            <div class="btn-group me-2" role="group">
                                <button
                                    :disabled="!data.reachable"
                                    type="button"
                                    class="btn btn-sm btn-danger"
                                    @click="onShowPowerModal()"
                                    v-tooltip
                                    :title="$t('gridcharger.TurnOnOff')"
                                >
                                    <BIconPower style="font-size: 24px" />
                                </button>
                            </div>
                        </div>
                    </div>

                    <div class="card-body">
                        <div class="row flex-row flex-wrap align-items-start g-3">
                            <div v-for="(values, section) in data.values" v-bind:key="section" class="col order-0">
                                <div class="card card-table" :class="{ 'border-info': section === 'device' }">
                                    <div :class="section === 'device' ? 'card-header text-bg-info' : 'card-header'">
                                        {{ $t('gridcharger.' + section) }}
                                    </div>
                                    <div class="card-body">
                                        <div class="table-responsive">
                                            <table class="table table-striped table-hover">
                                                <thead>
                                                    <tr>
                                                        <th scope="col">{{ $t('gridcharger.Property') }}</th>
                                                        <th class="value" scope="col">
                                                            {{ $t('gridcharger.Value') }}
                                                        </th>
                                                        <template v-if="section !== 'device'">
                                                            <th scope="col">{{ $t('gridcharger.Unit') }}</th>
                                                        </template>
                                                    </tr>
                                                </thead>
                                                <tbody>
                                                    <tr v-for="(prop, key) in values" v-bind:key="key">
                                                        <th scope="row">
                                                            {{ $t('gridcharger.' + key) }}
                                                        </th>
                                                        <td class="value">
                                                            <template v-if="isStringValue(prop) && prop.translate">
                                                                {{ $t('gridcharger.' + prop.value) }}
                                                            </template>
                                                            <template v-else-if="isStringValue(prop)">
                                                                {{ prop.value }}
                                                            </template>
                                                            <template v-else>
                                                                {{
                                                                    $n(prop.v, 'decimal', {
                                                                        minimumFractionDigits: prop.d,
                                                                        maximumFractionDigits: prop.d,
                                                                    })
                                                                }}
                                                            </template>
                                                        </td>
                                                        <td>
                                                            <template v-if="!isStringValue(prop)">
                                                                {{ prop.u }}
                                                            </template>
                                                        </td>
                                                    </tr>
                                                </tbody>
                                            </table>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <template v-if="data.provider === 0">
            <ModalDialog
                modalId="gridChargerLimitSettingView"
                :title="$t('gridcharger.LimitSettings')"
                :loading="dataLoading"
            >
                <BootstrapAlert v-model="showAlertLimit" :variant="alertTypeLimit">
                    {{ alertMessageLimit }}
                </BootstrapAlert>

                <InputElement
                    :label="$t('gridcharger.SetVoltageLimit')"
                    type="number"
                    step="0.01"
                    v-model="targetLimitList.voltage"
                    postfix="V"
                />

                <InputElement
                    :label="$t('gridcharger.SetCurrentLimit')"
                    type="number"
                    step="0.1"
                    v-model="targetLimitList.current"
                    postfix="A"
                />
                <template #footer>
                    <button type="button" class="btn btn-danger" @click="onSubmitLimit">
                        {{ $t('gridcharger.SetLimits') }}
                    </button>
                </template>
            </ModalDialog>

            <ModalDialog modalId="gridChargerPowerView" :title="$t('gridcharger.PowerControl')" :loading="dataLoading">
                <div class="d-grid gap-2 col-6 mx-auto">
                    <button type="button" class="btn btn-success" @click="onSetPower(true)">
                        <BIconToggleOn class="fs-4" />&nbsp;{{ $t('gridcharger.TurnOn') }}
                    </button>
                    <button type="button" class="btn btn-danger" @click="onSetPower(false)">
                        <BIconToggleOff class="fs-4" />&nbsp;{{ $t('gridcharger.TurnOff') }}
                    </button>
                </div>
            </ModalDialog>
        </template>
    </template>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import type { GridCharger } from '@/types/GridChargerDataStatus';
import type { GridChargerLimitConfig } from '@/types/GridChargerLimitConfig';
import { handleResponse, authHeader, authUrl } from '@/utils/authentication';
import { isStringValue } from '@/types/StringValue';

import * as bootstrap from 'bootstrap';
import { BIconSpeedometer, BIconPower, BIconToggleOn, BIconToggleOff } from 'bootstrap-icons-vue';
import DataAgeDisplay from '@/components/DataAgeDisplay.vue';
import InputElement from '@/components/InputElement.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import ModalDialog from '@/components/ModalDialog.vue';
import WebSocketService from '@/utils/websocketService';

export default defineComponent({
    components: {
        BIconSpeedometer,
        BIconPower,
        BIconToggleOn,
        BIconToggleOff,
        InputElement,
        DataAgeDisplay,
        BootstrapAlert,
        ModalDialog,
    },
    data() {
        return {
            socket: {} as WebSocketService,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            data: {} as GridCharger,
            isFirstFetchAfterConnect: true,
            targetLimitList: {} as GridChargerLimitConfig,
            gridChargerLimitSettingView: null as bootstrap.Modal | null,
            gridChargerPowerView: null as bootstrap.Modal | null,
            alertMessageLimit: '',
            alertTypeLimit: 'info',
            showAlertLimit: false,
        };
    },
    created() {
        this.getInitialData();
        this.initSocket();
        this.initDataAgeing();
    },
    unmounted() {
        this.socket?.close();
        clearInterval(this.dataAgeInterval);
        this.gridChargerLimitSettingView?.dispose();
        this.gridChargerPowerView?.dispose();
    },
    methods: {
        isStringValue,
        getInitialData() {
            console.log('Get initialData for GridCharger');
            this.dataLoading = true;

            fetch('/api/gridchargerlivedata/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.data = data;
                    this.dataLoading = false;
                });
        },
        handleMessage(event: MessageEvent) {
            console.log(event);
            this.data = JSON.parse(event.data);
            this.dataLoading = false;
        },
        initSocket() {
            console.log('Starting connection to GridCharger WebSocket Server');

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/gridchargerlivedata`;

            this.socket = new WebSocketService(webSocketUrl, {
                onMessage: this.handleMessage,
                onOpen: () => {
                    console.log('GridCharger WebSocket connected');
                },
                onClose: () => {
                    console.log('GridCharger WebSocket closed');
                },
            });

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.socket?.close();
            };

            this.socket?.connect();
        },
        initDataAgeing() {
            this.dataAgeInterval = setInterval(() => {
                if (this.data) {
                    this.data.dataAge += 1000;
                }
            }, 1000);
        },
        onShowLimitSettings() {
            this.gridChargerLimitSettingView = new bootstrap.Modal('#gridChargerLimitSettingView');
            this.gridChargerLimitSettingView?.show();
        },
        onShowPowerModal() {
            this.gridChargerPowerView = new bootstrap.Modal('#gridChargerPowerView');
            this.gridChargerPowerView?.show();
        },
        onSubmitLimit(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.targetLimitList));

            console.log(this.targetLimitList);

            fetch('/api/gridcharger/limit', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    if (response.type === 'success') {
                        this.gridChargerLimitSettingView?.hide();
                        this.showAlertLimit = false;
                    } else {
                        this.alertMessageLimit = this.$t('onbatteryapiresponse.' + response.code, response.param);
                        this.alertTypeLimit = response.type;
                        this.showAlertLimit = true;
                    }
                });
        },
        onSetPower(power: boolean) {
            const formData = new FormData();
            formData.append('data', JSON.stringify({ power }));

            fetch('/api/gridcharger/power', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    console.log(response);
                    if (response.type === 'success') {
                        this.gridChargerPowerView?.hide();
                    }
                });
        },
        getStatusClass() {
            if (this.data.reachable !== true) {
                return 'text-bg-danger';
            }

            if (this.data.reachable === true && this.data.producing === false) {
                return 'text-bg-warning';
            }

            return 'text-bg-success';
        },
    },
});
</script>
