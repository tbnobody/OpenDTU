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
                        :class="{
                            'text-bg-danger': huaweiData.data_age > 20,
                            'text-bg-primary': huaweiData.data_age < 19,
                        }"
                    >
                        <div class="p-1 flex-grow-1">
                            <div class="d-flex flex-wrap">
                                <div style="padding-right: 2em">Huawei R4850G2</div>
                                <div style="padding-right: 2em">
                                    {{ $t('huawei.DataAge') }} {{ $t('huawei.Seconds', { val: huaweiData.data_age }) }}
                                </div>
                            </div>
                        </div>
                        <div class="btn-toolbar p-2" role="toolbar">
                            <div class="btn-group me-2" role="group">
                                <button
                                    :disabled="false"
                                    type="button"
                                    class="btn btn-sm btn-danger"
                                    @click="onShowLimitSettings()"
                                    v-tooltip
                                    :title="$t('huawei.ShowSetLimit')"
                                >
                                    <BIconSpeedometer style="font-size: 24px" />
                                </button>
                            </div>
                        </div>
                    </div>

                    <div class="card-body">
                        <div class="row flex-row flex-wrap align-items-start g-3">
                            <div class="col order-0">
                                <div class="card" :class="{ 'border-info': true }">
                                    <div class="card-header bg-info">{{ $t('huawei.Input') }}</div>
                                    <div class="card-body">
                                        <table class="table table-striped table-hover">
                                            <thead>
                                                <tr>
                                                    <th scope="col">{{ $t('huawei.Property') }}</th>
                                                    <th style="text-align: right" scope="col">
                                                        {{ $t('huawei.Value') }}
                                                    </th>
                                                    <th scope="col">{{ $t('huawei.Unit') }}</th>
                                                </tr>
                                            </thead>
                                            <tbody>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.input_voltage') }}</th>
                                                    <td style="text-align: right">
                                                        {{ formatNumber(huaweiData.input_voltage.v) }}
                                                    </td>
                                                    <td>{{ huaweiData.input_voltage.u }}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.input_current') }}</th>
                                                    <td style="text-align: right">
                                                        {{ formatNumber(huaweiData.input_current.v) }}
                                                    </td>
                                                    <td>{{ huaweiData.input_current.u }}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.input_power') }}</th>
                                                    <td style="text-align: right">
                                                        {{ formatNumber(huaweiData.input_power.v) }}
                                                    </td>
                                                    <td>{{ huaweiData.input_power.u }}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.input_temp') }}</th>
                                                    <td style="text-align: right">
                                                        {{ Math.round(huaweiData.input_temp.v) }}
                                                    </td>
                                                    <td>{{ huaweiData.input_temp.u }}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.efficiency') }}</th>
                                                    <td style="text-align: right">
                                                        {{ huaweiData.efficiency.v.toFixed(1) }}
                                                    </td>
                                                    <td>{{ huaweiData.efficiency.u }}</td>
                                                </tr>
                                            </tbody>
                                        </table>
                                    </div>
                                </div>
                            </div>
                            <div class="col order-1">
                                <div class="card" :class="{ 'border-info': false }">
                                    <div class="card-header bg-info">{{ $t('huawei.Output') }}</div>
                                    <div class="card-body">
                                        <table class="table table-striped table-hover">
                                            <thead>
                                                <tr>
                                                    <th scope="col">{{ $t('huawei.Property') }}</th>
                                                    <th style="text-align: right" scope="col">
                                                        {{ $t('huawei.Value') }}
                                                    </th>
                                                    <th scope="col">{{ $t('huawei.Unit') }}</th>
                                                </tr>
                                            </thead>
                                            <tbody>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.output_voltage') }}</th>
                                                    <td style="text-align: right">
                                                        {{ huaweiData.output_voltage.v.toFixed(1) }}
                                                    </td>
                                                    <td>{{ huaweiData.output_voltage.u }}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.output_current') }}</th>
                                                    <td style="text-align: right">
                                                        {{ huaweiData.output_current.v.toFixed(2) }}
                                                    </td>
                                                    <td>{{ huaweiData.output_current.u }}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.max_output_current') }}</th>
                                                    <td style="text-align: right">
                                                        {{ huaweiData.max_output_current.v.toFixed(1) }}
                                                    </td>
                                                    <td>{{ huaweiData.max_output_current.u }}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.output_power') }}</th>
                                                    <td style="text-align: right">
                                                        {{ huaweiData.output_power.v.toFixed(1) }}
                                                    </td>
                                                    <td>{{ huaweiData.output_power.u }}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('huawei.output_temp') }}</th>
                                                    <td style="text-align: right">
                                                        {{ Math.round(huaweiData.output_temp.v) }}
                                                    </td>
                                                    <td>{{ huaweiData.output_temp.u }}</td>
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

        <div class="modal" id="huaweiLimitSettingView" ref="huaweiLimitSettingView" tabindex="-1">
            <div class="modal-dialog modal-lg">
                <div class="modal-content">
                    <form @submit="onSubmitLimit">
                        <div class="modal-header">
                            <h5 class="modal-title">{{ $t('huawei.LimitSettings') }}</h5>
                            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                        </div>
                        <div class="modal-body">
                            <div class="row mb-3">
                                <label for="inputCurrentLimit" class="col-sm-3 col-form-label"
                                    >{{ $t('huawei.CurrentLimit') }}
                                </label>
                            </div>

                            <div class="row mb-3 align-items-center">
                                <label for="inputVoltageTargetLimit" class="col-sm-3 col-form-label">{{
                                    $t('huawei.SetVoltageLimit')
                                }}</label>

                                <div class="col-sm-1">
                                    <div class="form-switch form-check-inline">
                                        <input
                                            class="form-check-input"
                                            type="checkbox"
                                            id="flexSwitchVoltage"
                                            v-model="targetLimitList.voltage_valid"
                                        />
                                    </div>
                                </div>

                                <div class="col-sm-7">
                                    <input
                                        type="number"
                                        step="0.01"
                                        name="inputVoltageTargetLimit"
                                        class="form-control"
                                        id="inputVoltageTargetLimit"
                                        :min="targetVoltageLimitMin"
                                        :max="targetVoltageLimitMax"
                                        v-model="targetLimitList.voltage"
                                        :disabled="!targetLimitList.voltage_valid"
                                    />
                                </div>
                            </div>

                            <div class="row mb-3">
                                <div class="col-sm-9">
                                    <div
                                        v-if="targetLimitList.voltage < targetVoltageLimitMinOffline"
                                        class="alert alert-secondary mt-3"
                                        role="alert"
                                        v-html="$t('huawei.LimitHint')"
                                    ></div>
                                </div>
                            </div>

                            <div class="row mb-3 align-items-center">
                                <label for="inputCurrentTargetLimit" class="col-sm-3 col-form-label">{{
                                    $t('huawei.SetCurrentLimit')
                                }}</label>

                                <div class="col-sm-1">
                                    <div class="form-switch form-check-inline">
                                        <input
                                            class="form-check-input"
                                            type="checkbox"
                                            id="flexSwitchCurrentt"
                                            v-model="targetLimitList.current_valid"
                                        />
                                    </div>
                                </div>

                                <div class="col-sm-7">
                                    <input
                                        type="number"
                                        step="0.1"
                                        name="inputCurrentTargetLimit"
                                        class="form-control"
                                        id="inputCurrentTargetLimit"
                                        :min="targetCurrentLimitMin"
                                        :max="targetCurrentLimitMax"
                                        v-model="targetLimitList.current"
                                        :disabled="!targetLimitList.current_valid"
                                    />
                                </div>
                            </div>
                        </div>

                        <div class="modal-footer">
                            <button type="submit" class="btn btn-danger" @click="onSetLimitSettings(true)">
                                {{ $t('huawei.SetOnline') }}
                            </button>

                            <button type="submit" class="btn btn-danger" @click="onSetLimitSettings(false)">
                                {{ $t('huawei.SetOffline') }}
                            </button>

                            <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">
                                {{ $t('huawei.Close') }}
                            </button>
                        </div>
                    </form>
                </div>
            </div>
        </div>
    </template>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import type { Huawei } from '@/types/HuaweiDataStatus';
import type { HuaweiLimitConfig } from '@/types/HuaweiLimitConfig';
import { handleResponse, authHeader, authUrl } from '@/utils/authentication';

import * as bootstrap from 'bootstrap';
import { BIconSpeedometer } from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BIconSpeedometer,
    },
    data() {
        return {
            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            huaweiData: {} as Huawei,
            isFirstFetchAfterConnect: true,
            targetVoltageLimitMin: 42,
            targetVoltageLimitMinOffline: 48,
            targetVoltageLimitMax: 58,
            targetCurrentLimitMin: 0,
            targetCurrentLimitMax: 60,
            targetLimitList: {} as HuaweiLimitConfig,
            targetLimitPersistent: false,
            huaweiLimitSettingView: {} as bootstrap.Modal,

            alertMessageLimit: '',
            alertTypeLimit: 'info',
            showAlertLimit: false,
            checked: false,
        };
    },
    created() {
        this.getInitialData();
        this.initSocket();
        this.initDataAgeing();
    },
    unmounted() {
        this.closeSocket();
    },
    methods: {
        getInitialData() {
            console.log('Get initalData for Huawei');
            this.dataLoading = true;

            fetch('/api/huaweilivedata/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.huaweiData = data;
                    this.dataLoading = false;
                });
        },
        initSocket() {
            console.log('Starting connection to Huawei WebSocket Server');

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/huaweilivedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);
                this.huaweiData = JSON.parse(event.data);
                this.dataLoading = false;
                this.heartCheck(); // Reset heartbeat detection
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log('Successfully connected to the Huawei websocket server...');
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        initDataAgeing() {
            this.dataAgeInterval = setInterval(() => {
                if (this.huaweiData) {
                    this.huaweiData.data_age++;
                }
            }, 1000);
        },
        // Send heartbeat packets regularly * 59s Send a heartbeat
        heartCheck() {
            this.heartInterval && clearTimeout(this.heartInterval);
            this.heartInterval = setInterval(() => {
                if (this.socket.readyState === 1) {
                    // Connection status
                    this.socket.send('ping');
                } else {
                    this.initSocket(); // Breakpoint reconnection 5 Time
                }
            }, 59 * 1000);
        },
        /** To break off websocket Connect */
        closeSocket() {
            this.socket.close();
            this.heartInterval && clearTimeout(this.heartInterval);
            this.isFirstFetchAfterConnect = true;
        },
        formatNumber(num: number) {
            return new Intl.NumberFormat(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 }).format(num);
        },
        onHideLimitSettings() {
            this.showAlertLimit = false;
        },
        onShowLimitSettings() {
            this.huaweiLimitSettingView = new bootstrap.Modal('#huaweiLimitSettingView');
            this.huaweiLimitSettingView.show();
        },
        onSetLimitSettings(online: boolean) {
            this.targetLimitList.online = online;
        },
        onSubmitLimit(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.targetLimitList));

            console.log(this.targetLimitList);

            fetch('/api/huawei/limit/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    if (response.type == 'success') {
                        this.huaweiLimitSettingView.hide();
                    } else {
                        this.alertMessageLimit = this.$t('apiresponse.' + response.code, response.param);
                        this.alertTypeLimit = response.type;
                        this.showAlertLimit = true;
                    }
                });
        },
    },
});
</script>
