<template>
    <div class="container-fluid" role="main">
        <div class="page-header">
            <h1>Live Data</h1>
        </div>

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
                        <button v-for="inverter in inverterData" :key="inverter.serial" class="nav-link"
                            :id="'v-pills-' + inverter.serial + '-tab'" data-bs-toggle="pill"
                            :data-bs-target="'#v-pills-' + inverter.serial" type="button" role="tab"
                            aria-controls="'v-pills-' + inverter.serial" aria-selected="true">
                            <BIconXCircleFill class="fs-4" v-if="!inverter.reachable" />
                            <BIconExclamationCircleFill class="fs-4" v-if="inverter.reachable && !inverter.producing" />
                            <BIconCheckCircleFill class="fs-4" v-if="inverter.reachable && inverter.producing" />
                            {{ inverter.name }}
                        </button>
                    </div>
                </div>

                <div class="tab-content col-sm-9 col-md-10" id="v-pills-tabContent">
                    <div v-for="inverter in inverterData" :key="inverter.serial" class="tab-pane fade show"
                        :id="'v-pills-' + inverter.serial" role="tabpanel"
                        :aria-labelledby="'v-pills-' + inverter.serial + '-tab'" tabindex="0">
                        <div class="card">
                            <div class="card-header text-white bg-primary d-flex justify-content-between align-items-center"
                                :class="{
                                    'bg-danger': !inverter.reachable,
                                    'bg-warning': inverter.reachable && !inverter.producing,
                                    'bg-primary': inverter.reachable && inverter.producing,
                                }">
                                {{ inverter.name }} (Inverter Serial Number:
                                {{ inverter.serial }}) (Data Age:
                                {{ inverter.data_age }} seconds)

                                <div class="btn-toolbar" role="toolbar">
                                    <div class="btn-group me-2" role="group">
                                        <button type="button" class="btn btn-sm btn-danger"
                                            @click="onShowLimitSettings(inverter.serial)"
                                            title="Show / Set Inverter Limit">
                                            <BIconSpeedometer style="font-size:24px;" />

                                        </button>
                                    </div>

                                    <div class="btn-group me-2" role="group">
                                        <button type="button" class="btn btn-sm btn-danger"
                                            @click="onShowPowerSettings(inverter.serial)" title="Turn Inverter on/off">
                                            <BIconPower style="font-size:24px;" />

                                        </button>
                                    </div>

                                    <div class="btn-group me-2" role="group">
                                        <button type="button" class="btn btn-sm btn-info"
                                            @click="onShowDevInfo(inverter.serial)" title="Show Inverter Info">
                                            <BIconCpu style="font-size:24px;" />

                                        </button>
                                    </div>

                                    <div class="btn-group" role="group">
                                        <button v-if="inverter.events >= 0" type="button"
                                            class="btn btn-sm btn-secondary position-relative"
                                            @click="onShowEventlog(inverter.serial)" title="Show Eventlog">
                                            <BIconJournalText style="font-size:24px;" />
                                            <span
                                                class="position-absolute top-0 start-100 translate-middle badge rounded-pill bg-danger">
                                                {{ inverter.events }}
                                                <span class="visually-hidden">unread messages</span>
                                            </span>
                                        </button>
                                    </div>
                                </div>
                            </div>
                            <div class="card-body">
                                <div class="row flex-row-reverse flex-wrap-reverse align-items-end g-3">
                                    <template v-for="channel in 5" :key="channel">
                                        <div v-if="inverter[channel - 1]" :class="`col order-${5 - channel}`">
                                            <InverterChannelInfo :channelData="inverter[channel - 1]"
                                                :channelNumber="channel - 1" />
                                        </div>
                                    </template>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </template>

        <div class="modal" id="eventView" tabindex="-1">
            <div class="modal-dialog modal-lg">
                <div class="modal-content">
                    <div class="modal-header">
                        <h5 class="modal-title">Event Log</h5>
                        <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                    </div>
                    <div class="modal-body">
                        <div class="text-center" v-if="eventLogLoading">
                            <div class="spinner-border" role="status">
                                <span class="visually-hidden">Loading...</span>
                            </div>
                        </div>

                        <EventLog v-if="!eventLogLoading" :eventLogList="eventLogList" />
                    </div>

                    <div class="modal-footer">
                        <button type="button" class="btn btn-secondary" @click="onHideEventlog"
                            data-bs-dismiss="modal">Close</button>
                    </div>

                </div>
            </div>
        </div>

        <div class="modal" id="devInfoView" tabindex="-1">
            <div class="modal-dialog modal-lg">
                <div class="modal-content">
                    <div class="modal-header">
                        <h5 class="modal-title">Inverter Info</h5>
                        <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                    </div>
                    <div class="modal-body">
                        <div class="text-center" v-if="devInfoLoading">
                            <div class="spinner-border" role="status">
                                <span class="visually-hidden">Loading...</span>
                            </div>
                        </div>

                        <DevInfo v-if="!devInfoLoading" :devInfoList="devInfoList" />
                    </div>

                    <div class="modal-footer">
                        <button type="button" class="btn btn-secondary" @click="onHideDevInfo"
                            data-bs-dismiss="modal">Close</button>
                    </div>
                </div>
            </div>
        </div>

        <div class="modal" id="limitSettingView" ref="limitSettingView" tabindex="-1">
            <div class="modal-dialog modal-lg">
                <div class="modal-content">
                    <form @submit="onSubmitLimit">
                        <div class="modal-header">
                            <h5 class="modal-title">Limit Settings</h5>
                            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                        </div>
                        <div class="modal-body">

                            <BootstrapAlert v-model="showAlertLimit" :variant="alertTypeLimit">
                                {{ alertMessageLimit }}
                            </BootstrapAlert>
                            <div class="text-center" v-if="limitSettingLoading">
                                <div class="spinner-border" role="status">
                                    <span class="visually-hidden">Loading...</span>
                                </div>
                            </div>

                            <template v-if="!limitSettingLoading">

                                <div class="row mb-3">
                                    <label for="inputCurrentLimit" class="col-sm-3 col-form-label">Current
                                        Limit:</label>
                                    <div class="col-sm-4">
                                        <div class="input-group">
                                            <input type="number" class="form-control" id="inputCurrentLimit"
                                                aria-describedby="currentLimitType" v-model="currentLimit" disabled />
                                            <span class="input-group-text" id="currentLimitType">%</span>
                                        </div>
                                    </div>

                                    <div class="col-sm-4" v-if="maxPower > 0">
                                        <div class="input-group">
                                            <input type="number" class="form-control" id="inputCurrentLimitAbsolute"
                                                aria-describedby="currentLimitTypeAbsolute"
                                                v-model="currentLimitAbsolute" disabled />
                                            <span class="input-group-text" id="currentLimitTypeAbsolute">W</span>
                                        </div>
                                    </div>
                                </div>

                                <div class="row mb-3 align-items-center">
                                    <label for="inputLastLimitSet" class="col-sm-3 col-form-label">Last Limit Set
                                        Status:</label>
                                    <div class="col-sm-9">
                                        <span class="badge" :class="{
                                            'bg-danger': successCommandLimit == 'Failure',
                                            'bg-warning': successCommandLimit == 'Pending',
                                            'bg-success': successCommandLimit == 'Ok',
                                            'bg-secondary': successCommandLimit == 'Unknown',
                                        }">
                                            {{ successCommandLimit }}
                                        </span>
                                    </div>
                                </div>

                                <div class="row mb-3">
                                    <label for="inputTargetLimit" class="col-sm-3 col-form-label">Set Limit:</label>
                                    <div class="col-sm-9">
                                        <div class="input-group">
                                            <input type="number" name="inputTargetLimit" class="form-control"
                                                id="inputTargetLimit" :min="targetLimitMin" :max="targetLimitMax"
                                                v-model="targetLimit">
                                            <button class="btn btn-primary dropdown-toggle" type="button"
                                                data-bs-toggle="dropdown" aria-expanded="false">{{ targetLimitTypeText
                                                }}</button>
                                            <ul class="dropdown-menu dropdown-menu-end">
                                                <li><a class="dropdown-item" @click="onSelectType(1)" href="#">Relative
                                                        (%)</a></li>
                                                <li><a class="dropdown-item" @click="onSelectType(0)" href="#">Absolute
                                                        (W)</a></li>
                                            </ul>
                                        </div>
                                        <div v-if="targetLimitType == 0" class="alert alert-secondary mt-3"
                                            role="alert">
                                            <b>Hint:</b> If you set the limit as absolute value the display of the
                                            current value will only be updated after ~4 minutes.
                                        </div>
                                    </div>
                                </div>
                            </template>

                        </div>

                        <div class="modal-footer">
                            <button type="submit" class="btn btn-danger" @click="onSetLimitSettings(true)">Set Limit
                                Persistent</button>

                            <button type="submit" class="btn btn-danger" @click="onSetLimitSettings(false)">Set Limit
                                Non-Persistent</button>

                            <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">Close</button>
                        </div>
                    </form>
                </div>
            </div>
        </div>

        <div class="modal" id="powerSettingView" ref="powerSettingView" tabindex="-1">
            <div class="modal-dialog modal-lg">
                <div class="modal-content">
                    <div class="modal-header">
                        <h5 class="modal-title">Power Settings</h5>
                        <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                    </div>
                    <div class="modal-body">

                        <BootstrapAlert v-model="showAlertPower" :variant="alertTypePower">
                            {{ alertMessagePower }}
                        </BootstrapAlert>
                        <div class="text-center" v-if="powerSettingLoading">
                            <div class="spinner-border" role="status">
                                <span class="visually-hidden">Loading...</span>
                            </div>
                        </div>

                        <template v-if="!powerSettingLoading">
                            <div class="row mb-3 align-items-center">
                                <label for="inputLastPowerSet" class="col col-form-label">Last Power Set
                                    Status:</label>
                                <div class="col">
                                    <span class="badge" :class="{
                                        'bg-danger': successCommandPower == 'Failure',
                                        'bg-warning': successCommandPower == 'Pending',
                                        'bg-success': successCommandPower == 'Ok',
                                        'bg-secondary': successCommandPower == 'Unknown',
                                    }">
                                        {{ successCommandPower }}
                                    </span>
                                </div>
                            </div>

                            <div class="d-grid gap-2 col-6 mx-auto">
                                <button type="button" class="btn btn-success" @click="onSetPowerSettings(true)">
                                    <BIconToggleOn class="fs-4" />&nbsp;Turn On
                                </button>
                                <button type="button" class="btn btn-danger" @click="onSetPowerSettings(false)">
                                    <BIconToggleOff class="fs-4" />&nbsp;Turn Off
                                </button>
                                <button type="button" class="btn btn-warning" @click="onSetPowerSettings(true, true)">
                                    <BIconArrowCounterclockwise class="fs-4" />&nbsp;Restart
                                </button>
                            </div>
                        </template>

                    </div>

                    <div class="modal-footer">
                        <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">Close</button>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import InverterChannelInfo from "@/components/partials/InverterChannelInfo.vue";
import * as bootstrap from 'bootstrap';
import EventLog from '@/components/partials/EventLog.vue';
import DevInfo from '@/components/partials/DevInfo.vue';
import BootstrapAlert from '@/components/partials/BootstrapAlert.vue';

declare interface Inverter {
    serial: number,
    name: string,
    reachable: boolean,
    producing: boolean,
    data_age: 0,
    events: 0
}

export default defineComponent({
    components: {
        InverterChannelInfo,
        EventLog,
        DevInfo,
        BootstrapAlert,
    },
    data() {
        return {
            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            inverterData: [] as Inverter[],
            isFirstFetchAfterConnect: true,
            eventLogView: {} as bootstrap.Modal,
            eventLogList: {},
            eventLogLoading: true,
            devInfoView: {} as bootstrap.Modal,
            devInfoList: {},
            devInfoLoading: true,

            limitSettingView: {} as bootstrap.Modal,
            limitSettingSerial: 0,
            limitSettingLoading: true,

            currentLimit: 0,
            currentLimitAbsolute: 0,
            successCommandLimit: "",
            maxPower: 0,
            targetLimit: 0,
            targetLimitMin: 10,
            targetLimitMax: 100,
            targetLimitTypeText: "Relative (%)",
            targetLimitType: 1,
            targetLimitPersistent: false,

            alertMessageLimit: "",
            alertTypeLimit: "info",
            showAlertLimit: false,

            powerSettingView: {} as bootstrap.Modal,
            powerSettingSerial: 0,
            powerSettingLoading: true,
            alertMessagePower: "",
            alertTypePower: "info",
            showAlertPower: false,
            successCommandPower: "",
        };
    },
    created() {
        this.getInitialData();
        this.initSocket();
        this.initDataAgeing();
    },
    mounted() {
        this.eventLogView = new bootstrap.Modal('#eventView');
        this.devInfoView = new bootstrap.Modal('#devInfoView');
        this.limitSettingView = new bootstrap.Modal('#limitSettingView');
        this.powerSettingView = new bootstrap.Modal('#powerSettingView');

        (this.$refs.limitSettingView as HTMLElement).addEventListener("hide.bs.modal", this.onHideLimitSettings);
        (this.$refs.powerSettingView as HTMLElement).addEventListener("hide.bs.modal", this.onHidePowerSettings);
    },
    unmounted() {
        this.closeSocket();
    },
    updated() {
        // Select first tab
        if (this.isFirstFetchAfterConnect) {
            this.isFirstFetchAfterConnect = false;

            const firstTabEl = this.$el.querySelector(
                "#v-pills-tab:first-child button"
            );
            if (firstTabEl != null) {
                const firstTab = new bootstrap.Tab(firstTabEl);
                firstTab.show();
            }
        }
    },
    methods: {
        getInitialData() {
            this.dataLoading = true;
            fetch("/api/livedata/status")
                .then((response) => response.json())
                .then((data) => {
                    this.inverterData = data;
                    this.dataLoading = false;
                });
        },
        initSocket() {
            console.log("Starting connection to WebSocket Server");

            const { protocol, host } = location;
            const webSocketUrl = `${protocol === "https:" ? "wss" : "ws"
                }://${host}/livedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);
                this.inverterData = JSON.parse(event.data);
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
                this.inverterData.forEach(element => {
                    element.data_age++;
                });
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
        onHideEventlog() {
            this.eventLogView.hide();
        },
        onShowEventlog(serial: number) {
            this.eventLogLoading = true;
            fetch("/api/eventlog/status?inv=" + serial)
                .then((response) => response.json())
                .then((data) => {
                    this.eventLogList = data[serial];
                    this.eventLogLoading = false;
                });

            this.eventLogView.show();
        },
        onHideDevInfo() {
            this.devInfoView.hide();
        },
        onShowDevInfo(serial: number) {
            this.devInfoLoading = true;
            fetch("/api/devinfo/status")
                .then((response) => response.json())
                .then((data) => {
                    this.devInfoList = data[serial][0];
                    this.devInfoLoading = false;
                });

            this.devInfoView.show();
        },
        onHideLimitSettings() {
            this.limitSettingSerial = 0;
            this.targetLimit = 0;
            this.targetLimitType = 1;
            this.targetLimitTypeText = "Relative (%)";
            this.showAlertLimit = false;
        },
        onShowLimitSettings(serial: number) {
            this.limitSettingLoading = true;
            fetch("/api/limit/status")
                .then((response) => response.json())
                .then((data) => {
                    this.maxPower = data[serial].max_power;
                    this.currentLimit = data[serial].limit_relative;
                    if (this.maxPower > 0) {
                        this.currentLimitAbsolute = this.currentLimit * this.maxPower / 100;
                    }
                    this.successCommandLimit = data[serial].limit_set_status;
                    this.limitSettingSerial = serial;
                    this.limitSettingLoading = false;
                });

            this.limitSettingView.show();
        },
        onSubmitLimit(e: Event) {
            e.preventDefault();

            const data = {
                serial: this.limitSettingSerial,
                limit_value: this.targetLimit,
                limit_type: (this.targetLimitPersistent ? 256 : 0) + this.targetLimitType,
            };
            const formData = new FormData();
            formData.append("data", JSON.stringify(data));

            console.log(data);

            fetch("/api/limit/config", {
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
                        if (response.type == "success") {
                            this.limitSettingView.hide();
                        } else {
                            this.alertMessageLimit = response.message;
                            this.alertTypeLimit = response.type;
                            this.showAlertLimit = true;
                        }
                    }
                )
        },
        onSetLimitSettings(setPersistent: boolean) {
            this.targetLimitPersistent = setPersistent;
        },
        onSelectType(type: number) {
            if (type == 1) {
                this.targetLimitTypeText = "Relative (%)";
                this.targetLimitMin = 10;
                this.targetLimitMax = 100;
            } else {
                this.targetLimitTypeText = "Absolute (W)";
                this.targetLimitMin = 10;
                this.targetLimitMax = 1500;
            }
            this.targetLimitType = type;
        },

        onShowPowerSettings(serial: number) {
            this.powerSettingLoading = true;
            fetch("/api/power/status")
                .then((response) => response.json())
                .then((data) => {
                    this.successCommandPower = data[serial].power_set_status;
                    this.powerSettingSerial = serial;
                    this.powerSettingLoading = false;
                });
            this.powerSettingView.show();
        },

        onHidePowerSettings() {
            this.powerSettingSerial = 0;
            this.showAlertPower = false;
        },

        onSetPowerSettings(turnOn: boolean, restart = false) {
            let data = {};
            if (restart) {
                data = {
                    serial: this.powerSettingSerial,
                    restart: true,
                };
            } else {
                data = {
                    serial: this.powerSettingSerial,
                    power: turnOn,
                };
            }

            const formData = new FormData();
            formData.append("data", JSON.stringify(data));

            console.log(data);

            fetch("/api/power/config", {
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
                        if (response.type == "success") {
                            this.powerSettingView.hide();
                        } else {
                            this.alertMessagePower = response.message;
                            this.alertTypePower = response.type;
                            this.showAlertPower = true;
                        }
                    }
                )
        },
    },
});
</script>