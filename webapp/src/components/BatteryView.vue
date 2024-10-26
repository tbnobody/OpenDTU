<template>
    <div class="text-center" v-if="dataLoading">
        <div class="spinner-border" role="status">
            <span class="visually-hidden">Loading...</span>
        </div>
    </div>

    <div v-else-if="'values' in batteryData">
        <div class="row gy-3 mt-0">
            <div class="tab-content col-sm-12 col-md-12" id="v-pills-tabContent">
                <div class="card">
                    <div
                        class="card-header d-flex justify-content-between align-items-center"
                        :class="{
                            'text-bg-danger': batteryData.data_age >= 20,
                            'text-bg-primary': batteryData.data_age < 20,
                        }"
                    >
                        <div class="p-1 flex-grow-1">
                            <div class="d-flex flex-wrap">
                                <div style="padding-right: 2em">
                                    {{ $t('battery.battery') }}: {{ batteryData.manufacturer }}
                                </div>
                                <div style="padding-right: 2em" v-if="'serial' in batteryData">
                                    {{ $t('home.SerialNumber') }}{{ batteryData.serial }}
                                </div>
                                <div style="padding-right: 2em" v-if="'fwversion' in batteryData">
                                    {{ $t('battery.FwVersion') }}: {{ batteryData.fwversion }}
                                </div>
                                <div style="padding-right: 2em" v-if="'hwversion' in batteryData">
                                    {{ $t('battery.HwVersion') }}: {{ batteryData.hwversion }}
                                </div>
                                <div style="padding-right: 2em">
                                    {{ $t('battery.DataAge') }}
                                    {{ $t('battery.Seconds', { val: batteryData.data_age }) }}
                                </div>
                            </div>
                        </div>
                    </div>

                    <div class="card-body">
                        <div class="row flex-row flex-wrap align-items-start g-3">
                            <div
                                v-for="(values, section) in batteryData.values"
                                v-bind:key="section"
                                class="col order-0"
                            >
                                <div class="card card-table" :class="{ 'border-info': true }">
                                    <div class="card-header text-bg-info">{{ $t('battery.' + section) }}</div>
                                    <div class="card-body">
                                        <div class="table-responsive">
                                            <table class="table table-striped table-hover">
                                                <thead>
                                                    <tr>
                                                        <th scope="col">{{ $t('battery.Property') }}</th>
                                                        <th style="text-align: right" scope="col">
                                                            {{ $t('battery.Value') }}
                                                        </th>
                                                        <th scope="col">{{ $t('battery.Unit') }}</th>
                                                    </tr>
                                                </thead>
                                                <tbody>
                                                    <tr v-for="(prop, key) in values" v-bind:key="key">
                                                        <th scope="row">{{ $t('battery.' + key) }}</th>
                                                        <td style="text-align: right">
                                                            <template v-if="isStringValue(prop) && prop.translate">
                                                                {{ $t('battery.' + prop.value) }}
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
                            <div class="col order-1" v-show="batteryData.showIssues">
                                <div class="card card-table">
                                    <div :class="{ 'card-header': true, 'border-bottom-0': maxIssueValue === 0 }">
                                        <div class="d-flex flex-row justify-content-between align-items-baseline">
                                            {{ $t('battery.issues') }}
                                            <div v-if="maxIssueValue === 0" class="badge text-bg-success">
                                                {{ $t('battery.noIssues') }}
                                            </div>
                                            <div
                                                v-else-if="maxIssueValue === 1"
                                                class="badge text-bg-warning text-dark"
                                            >
                                                {{ $t('battery.warning') }}
                                            </div>
                                            <div v-else-if="maxIssueValue === 2" class="badge text-bg-danger">
                                                {{ $t('battery.alarm') }}
                                            </div>
                                        </div>
                                    </div>
                                    <div class="card-body" v-if="'issues' in batteryData">
                                        <table class="table table-striped table-hover">
                                            <thead>
                                                <tr>
                                                    <th scope="col">{{ $t('battery.issueName') }}</th>
                                                    <th scope="col">{{ $t('battery.issueType') }}</th>
                                                </tr>
                                            </thead>
                                            <tbody>
                                                <tr v-for="(prop, key) in batteryData.issues" v-bind:key="key">
                                                    <th scope="row">{{ $t('battery.' + key) }}</th>
                                                    <td>
                                                        <span
                                                            class="badge"
                                                            :class="{
                                                                'text-bg-warning text-dark': prop === 1,
                                                                'text-bg-danger': prop === 2,
                                                            }"
                                                        >
                                                            <template v-if="prop === 1">{{
                                                                $t('battery.warning')
                                                            }}</template>
                                                            <template v-else>{{ $t('battery.alarm') }}</template>
                                                        </span>
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
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import type { Battery, StringValue } from '@/types/BatteryDataStatus';
import type { ValueObject } from '@/types/LiveDataStatus';
import { handleResponse, authHeader, authUrl } from '@/utils/authentication';

export default defineComponent({
    components: {},
    data() {
        return {
            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            batteryData: {} as Battery,
            isFirstFetchAfterConnect: true,

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
        isStringValue(value: ValueObject | StringValue): value is StringValue {
            return value && typeof value === 'object' && 'translate' in value;
        },
        getInitialData() {
            console.log('Get initalData for Battery');
            this.dataLoading = true;

            fetch('/api/batterylivedata/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.batteryData = data;
                    this.dataLoading = false;
                });
        },
        initSocket() {
            console.log('Starting connection to Battery WebSocket Server');

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/batterylivedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);
                this.batteryData = JSON.parse(event.data);
                this.dataLoading = false;
                this.heartCheck(); // Reset heartbeat detection
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log('Successfully connected to the Battery websocket server...');
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        initDataAgeing() {
            this.dataAgeInterval = setInterval(() => {
                if (this.batteryData) {
                    this.batteryData.data_age++;
                }
            }, 1000);
        },
        // Send heartbeat packets regularly * 59s Send a heartbeat
        heartCheck() {
            if (this.heartInterval) {
                clearTimeout(this.heartInterval);
            }
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
            if (this.heartInterval) {
                clearTimeout(this.heartInterval);
            }
            this.isFirstFetchAfterConnect = true;
        },
    },
    computed: {
        maxIssueValue() {
            return 'issues' in this.batteryData ? Math.max(...Object.values(this.batteryData.issues)) : 0;
        },
    },
});
</script>
