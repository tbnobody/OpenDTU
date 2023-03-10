<template>

    <div class="text-center" v-if="dataLoading">
        <div class="spinner-border" role="status">
            <span class="visually-hidden">Loading...</span>
        </div>
    </div>

    <template v-else>
        <div class="row gy-3">
            <div class="tab-content col-sm-12 col-md-12" id="v-pills-tabContent">
                <div class="card">
                    <div class="card-header d-flex justify-content-between align-items-center"
                        :class="{
                            'text-bg-danger': vedirectData.age_critical,
                            'text-bg-primary': !vedirectData.age_critical,
                        }">
                        <div class="p-1 flex-grow-1">
                            <div class="d-flex flex-wrap">
                                <div style="padding-right: 2em;">
                                    {{ vedirectData.PID }}
                                </div>
                                <div style="padding-right: 2em;">
                                    {{ $t('vedirecthome.SerialNumber') }} {{ vedirectData.SER }}
                                </div>
                                <div style="padding-right: 2em;">
                                    {{ $t('vedirecthome.FirmwareNumber') }}  {{ vedirectData.FW }}
                                </div>
                                <div style="padding-right: 2em;">
                                    {{ $t('vedirecthome.DataAge') }} {{ $t('vedirecthome.Seconds', {'val': vedirectData.data_age }) }}
                                </div>
                            </div>
                        </div>
                    </div>
                    <div class="card-body">
                        <div class="row flex-row flex-wrap align-items-start g-3">
                            <div class="col order-0">
                                <div class="card" :class="{ 'border-info': true }">
                                    <div class="card-header text-bg-info">{{ $t('vedirecthome.DeviceInfo') }}</div>
                                    <div class="card-body">
                                        <table class="table table-striped table-hover">
                                            <thead>
                                                <tr>
                                                    <th scope="col">{{ $t('vedirecthome.Property') }}</th>
                                                    <th style="text-align: right" scope="col">{{ $t('vedirecthome.Value') }}</th>
                                                    <th scope="col">{{ $t('vedirecthome.Unit') }}</th>
                                                </tr>
                                            </thead>
                                            <tbody>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.LoadOutputState') }}</th>
                                                    <td style="text-align: right">{{vedirectData.LOAD}}</td>
                                                    <td></td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.StateOfOperation') }}</th>
                                                    <td style="text-align: right">{{vedirectData.CS}}</td>
                                                    <td></td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.TrackerOperationMode') }}</th>
                                                    <td style="text-align: right">{{vedirectData.MPPT}}</td>
                                                    <td></td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.OffReason') }}</th>
                                                    <td style="text-align: right">{{vedirectData.OR}}</td>
                                                    <td></td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.ErrorCode') }}</th>
                                                    <td style="text-align: right">{{vedirectData.ERR}}</td>
                                                    <td></td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.DaySequenceNumber') }}</th>
                                                    <td style="text-align: right">{{vedirectData.HSDS.v}}</td>
                                                    <td>{{vedirectData.HSDS.u}}</td>
                                                </tr>
                                            </tbody>
                                        </table>
                                    </div>
                                </div>
                            </div>
                            <div class="col order-1">
                                <div class="card" :class="{ 'border-info': false }">
                                    <div class="card-header">{{ $t('vedirecthome.Battery') }}</div>
                                    <div class="card-body">
                                        <table class="table table-striped table-hover">
                                            <thead>
                                                <tr>
                                                    <th scope="col">{{ $t('vedirecthome.Property') }}</th>
                                                    <th style="text-align: right" scope="col">{{ $t('vedirecthome.Value') }}</th>
                                                    <th scope="col">{{ $t('vedirecthome.Unit') }}</th>
                                                </tr>
                                            </thead>
                                            <tbody>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.BatteryVoltage') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.V.v)}}</td>
                                                    <td>{{vedirectData.V.u}}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.BatteryCurrent') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.I.v)}}</td>
                                                    <td>{{vedirectData.I.u}}</td>
                                                </tr>
                                            </tbody>
                                        </table>
                                    </div>
                                </div>
                            </div>
                            <div class="col order-2">
                                <div class="card" :class="{ 'border-info': false }">
                                    <div class="card-header">{{ $t('vedirecthome.Panel') }}</div>
                                    <div class="card-body">
                                        <table class="table table-striped table-hover">
                                            <thead>
                                                <tr>
                                                    <th scope="col">{{ $t('vedirecthome.Property') }}</th>
                                                    <th style="text-align: right" scope="col">{{ $t('vedirecthome.Value') }}</th>
                                                    <th scope="col">{{ $t('vedirecthome.Unit') }}</th>
                                                </tr>
                                            </thead>
                                            <tbody>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.PanelVoltage') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.VPV.v)}}</td>
                                                    <td>{{vedirectData.VPV.u}}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.PanelPower') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.PPV.v)}}</td>
                                                    <td>{{vedirectData.PPV.u}}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.YieldTotal') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.H19.v)}}</td>
                                                    <td>{{vedirectData.H19.u}}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.YieldToday') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.H20.v)}}</td>
                                                    <td>{{vedirectData.H20.u}}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.MaximumPowerToday') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.H21.v)}}</td>
                                                    <td>{{vedirectData.H21.u}}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.YieldYesterday') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.H22.v)}}</td>
                                                    <td>{{vedirectData.H22.u}}</td>
                                                </tr>
                                                <tr>
                                                    <th scope="row">{{ $t('vedirecthome.MaximumPowerYesterday') }}</th>
                                                    <td style="text-align: right">{{formatNumber(vedirectData.H23.v)}}</td>
                                                    <td>{{vedirectData.H23.u}}</td>
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
    </template>



</template>

<script lang="ts">
import { defineComponent } from 'vue';
import type { Vedirect } from '@/types/VedirectLiveDataStatus';
import { handleResponse, authHeader, authUrl } from '@/utils/authentication';


export default defineComponent({
    components: {
    },
    data() {
        return {
            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            vedirectData: {} as Vedirect,
            isFirstFetchAfterConnect: true,
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
            console.log("Get initalData for VeDirect");
            this.dataLoading = true;
            fetch("/api/vedirectlivedata/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.vedirectData = data;
                    this.dataLoading = false;
                });
        },
        initSocket() {
            console.log("Starting connection to VeDirect WebSocket Server");

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === "https:" ? "wss" : "ws"
                }://${authString}${host}/vedirectlivedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);
                this.vedirectData = JSON.parse(event.data);
                this.dataLoading = false;
                this.heartCheck(); // Reset heartbeat detection
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log("Successfully connected to the VeDirect websocket server...");
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        initDataAgeing() {
            this.dataAgeInterval = setInterval(() => {
                if (this.vedirectData) {
                    this.vedirectData.data_age++;
                }
            }, 1000);
        },
        // Send heartbeat packets regularly * 59s Send a heartbeat
        heartCheck() {
            this.heartInterval && clearTimeout(this.heartInterval);
            this.heartInterval = setInterval(() => {
                if (this.socket.readyState === 1) {
                    // Connection status
                    this.socket.send("ping");
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
            return new Intl.NumberFormat(
                undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 }
            ).format(num);
        },
    },
});
</script>