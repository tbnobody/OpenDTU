<template>

    <div class="text-center" v-if="dataLoading">
        <div class="spinner-border" role="status">
            <span class="visually-hidden">Loading...</span>
        </div>
    </div>

    <template v-else>
        <div class="row gy-3">
            <div class="col-sm-3 col-md-2">
                <div class="nav nav-pills row-cols-sm-1" id="v-pills-tab" role="tablist"
                    aria-orientation="vertical">
                    <button class="nav-link"
                        :id="'v-pills-vedirect-tab'" data-bs-toggle="pill"
                        :data-bs-target="'#v-pills-vedirect'" type="button" role="tab"
                        aria-controls="'v-pills-vedirect'" aria-selected="true">
                        Ve.Direct
                    </button>
                </div>
            </div>

            <div class="tab-content col-sm-9 col-md-10" id="v-pills-tabContent">
                <div class="tab-pane fade show"
                    :id="'v-pills-vedirect'" role="tabpanel"
                    :aria-labelledby="'v-pills-vedirect-tab'" tabindex="0">
                    <div class="card">
                        <div class="card-header text-white bg-primary d-flex justify-content-between align-items-center"
                            :class="{
                                'bg-danger': vedirectData.age_critical,
                                'bg-primary': !vedirectData.age_critical,
                            }">
                            {{ vedirectData.PID }} (Serial Number:
                            {{ vedirectData.SER }}, Firmware 
                            {{ vedirectData.FW }}) (Data Age:
                            {{ vedirectData.data_age }} seconds)

                        </div>
                        <div class="card-body">
                            <div class="row">
                                <div class="col order-0">
                                    <div class="card" :class="{ 'border-info': true }">
                                        <div class="card-header bg-info">Device Info</div>
                                        <div class="card-body">
                                            <table class="table table-striped table-hover">
                                                <thead>
                                                    <tr>
                                                        <th scope="col">Property</th>
                                                        <th style="text-align: right" scope="col">Value</th>
                                                        <th scope="col">Unit</th>
                                                    </tr>
                                                </thead>
                                                <tbody>
                                                    <tr>
                                                        <th scope="row">Load output state </th>
                                                        <td style="text-align: right">{{vedirectData.LOAD}}</td>
                                                        <td></td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">State of operation </th>
                                                        <td style="text-align: right">{{vedirectData.CS}}</td>
                                                        <td></td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Tracker operation mode </th>
                                                        <td style="text-align: right">{{vedirectData.MPPT}}</td>
                                                        <td></td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Off reason </th>
                                                        <td style="text-align: right">{{vedirectData.OR}}</td>
                                                        <td></td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Error code </th>
                                                        <td style="text-align: right">{{vedirectData.ERR}}</td>
                                                        <td></td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Day sequence number (0..364) </th>
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
                                        <div class="card-header">Battery</div>
                                        <div class="card-body">
                                            <table class="table table-striped table-hover">
                                                <thead>
                                                    <tr>
                                                        <th scope="col">Property</th>
                                                        <th style="text-align: right" scope="col">Value</th>
                                                        <th scope="col">Unit</th>
                                                    </tr>
                                                </thead>
                                                <tbody>
                                                    <tr>
                                                        <th scope="row">Battery voltage </th>
                                                        <td style="text-align: right">{{formatNumber(vedirectData.V.v)}}</td>
                                                        <td>{{vedirectData.V.u}}</td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Battery current </th>
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
                                        <div class="card-header">Panel</div>
                                        <div class="card-body">
                                            <table class="table table-striped table-hover">
                                                <thead>
                                                    <tr>
                                                        <th scope="col">Property</th>
                                                        <th style="text-align: right" scope="col">Value</th>
                                                        <th scope="col">Unit</th>
                                                    </tr>
                                                </thead>
                                                <tbody>
                                                    <tr>
                                                        <th scope="row">Panel voltage </th>
                                                        <td style="text-align: right">{{formatNumber(vedirectData.VPV.v)}}</td>
                                                        <td>{{vedirectData.VPV.u}}</td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Panel power </th>
                                                        <td style="text-align: right">{{formatNumber(vedirectData.PPV.v)}}</td>
                                                        <td>{{vedirectData.PPV.u}}</td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Yield total (user resettable counter) </th>
                                                        <td style="text-align: right">{{formatNumber(vedirectData.H19.v)}}</td>
                                                        <td>{{vedirectData.H19.u}}</td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Yield today </th>
                                                        <td style="text-align: right">{{formatNumber(vedirectData.H20.v)}}</td>
                                                        <td>{{vedirectData.H20.u}}</td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Maximum power today </th>
                                                        <td style="text-align: right">{{formatNumber(vedirectData.H21.v)}}</td>
                                                        <td>{{vedirectData.H21.u}}</td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Yield yesterday </th>
                                                        <td style="text-align: right">{{formatNumber(vedirectData.H22.v)}}</td>
                                                        <td>{{vedirectData.H22.u}}</td>
                                                    </tr>
                                                    <tr>
                                                        <th scope="row">Maximum power yesterday </th>
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
        </div>
    </template>



</template>

<script lang="ts">
import { defineComponent } from 'vue';

declare interface Vedirect {
    SER: string,
    PID: string,
    FW: string,
    age_critical: boolean,
    data_age: 0,
}

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
            this.dataLoading = true;
            fetch("/api/vedirectlivedata/status")
                .then((response) => response.json())
                .then((data) => {
                    this.vedirectData = data;
                    this.dataLoading = false;
                });
        },
        initSocket() {
            console.log("Starting connection to WebSocket Server");

            const { protocol, host } = location;
            const webSocketUrl = `${protocol === "https:" ? "wss" : "ws"
                }://${host}/vedirectlivedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);
                this.vedirectData = JSON.parse(event.data);
                this.dataLoading = false;
                this.heartCheck(); // Reset heartbeat detection
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log("Successfully connected to the echo websocket server...");
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        initDataAgeing() {
            this.dataAgeInterval = setInterval(() => {
                this.vedirectData.data_age++;
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
        formatNumber(num: string) {
            return new Intl.NumberFormat(
                undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 }
            ).format(parseFloat(num));
        },
    },
});
</script>