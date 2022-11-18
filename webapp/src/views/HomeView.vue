<template>
    <BasePage :title="'Live Data'" :isLoading="dataLoading" :isWideScreen="true">
        <InverterTotalInfo :totalData="liveData.total" /><br />
        <div class="row gy-3">
            <div class="col-sm-3 col-md-2" :style="[inverterData.length == 1 ? { 'display': 'none' } : {}]">
                <div class="nav nav-pills row-cols-sm-1" id="v-pills-tab" role="tablist" aria-orientation="vertical">
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

            <div class="tab-content" id="v-pills-tabContent" :class="{
                'col-sm-9 col-md-10': inverterData.length > 1,
                'col-sm-12 col-md-12': inverterData.length == 1
            }">
                <div v-for="inverter in inverterData" :key="inverter.serial" class="tab-pane fade show"
                    :id="'v-pills-' + inverter.serial" role="tabpanel"
                    :aria-labelledby="'v-pills-' + inverter.serial + '-tab'" tabindex="0">
                    <div class="card">
                        <div class="card-header d-flex justify-content-between align-items-center"
                            :class="{
                                'text-bg-danger': !inverter.reachable,
                                'text-bg-warning': inverter.reachable && !inverter.producing,
                                'text-bg-primary': inverter.reachable && inverter.producing,
                            }">
                            <div class="p-1 flex-grow-1">
                                <div class="d-flex flex-wrap">
                                    <div style="padding-right: 2em;">
                                        {{ inverter.name }}
                                    </div>
                                    <div style="padding-right: 2em;">
                                        Serial Number: {{ inverter.serial }}
                                    </div>
                                    <div style="padding-right: 2em;">
                                        Current Limit: <template v-if="inverter.limit_absolute > -1"> {{
                                                formatNumber(inverter.limit_absolute, 0)
                                        }} W | </template>{{ formatNumber(inverter.limit_relative, 0) }} %
                                    </div>
                                    <div style="padding-right: 2em;">
                                        Data Age: {{ inverter.data_age }} seconds
                                    </div>
                                </div>
                            </div>
                            <div class="btn-toolbar p-2" role="toolbar">
                                <div class="btn-group me-2" role="group">
                                    <button :disabled="!isLogged" type="button" class="btn btn-sm btn-danger"
                                        @click="onShowLimitSettings(inverter.serial)" title="Show / Set Inverter Limit">
                                        <BIconSpeedometer style="font-size:24px;" />

                                    </button>
                                </div>

                                <div class="btn-group me-2" role="group">
                                    <button :disabled="!isLogged" type="button" class="btn btn-sm btn-danger"
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
                                            class="position-absolute top-0 start-100 translate-middle badge rounded-pill text-bg-danger">
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
    </BasePage>

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
                                        <input type="text" class="form-control" id="inputCurrentLimit"
                                            aria-describedby="currentLimitType" v-model="currentLimitRelative"
                                            disabled />
                                        <span class="input-group-text" id="currentLimitType">%</span>
                                    </div>
                                </div>

                                <div class="col-sm-4" v-if="currentLimitList.max_power > 0">
                                    <div class="input-group">
                                        <input type="text" class="form-control" id="inputCurrentLimitAbsolute"
                                            aria-describedby="currentLimitTypeAbsolute" v-model="currentLimitAbsolute"
                                            disabled />
                                        <span class="input-group-text" id="currentLimitTypeAbsolute">W</span>
                                    </div>
                                </div>
                            </div>

                            <div class="row mb-3 align-items-center">
                                <label for="inputLastLimitSet" class="col-sm-3 col-form-label">Last Limit Set
                                    Status:</label>
                                <div class="col-sm-9">
                                    <span class="badge" :class="{
                                        'text-bg-danger': currentLimitList.limit_set_status == 'Failure',
                                        'text-bg-warning': currentLimitList.limit_set_status == 'Pending',
                                        'text-bg-success': currentLimitList.limit_set_status == 'Ok',
                                        'text-bg-secondary': currentLimitList.limit_set_status == 'Unknown',
                                    }">
                                        {{ currentLimitList.limit_set_status }}
                                    </span>
                                </div>
                            </div>

                            <div class="row mb-3">
                                <label for="inputTargetLimit" class="col-sm-3 col-form-label">Set Limit:</label>
                                <div class="col-sm-9">
                                    <div class="input-group">
                                        <input type="number" name="inputTargetLimit" class="form-control"
                                            id="inputTargetLimit" :min="targetLimitMin" :max="targetLimitMax"
                                            v-model="targetLimitList.limit_value">
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
                                    <div v-if="targetLimitType == 0" class="alert alert-secondary mt-3" role="alert">
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
                                    'text-bg-danger': successCommandPower == 'Failure',
                                    'text-bg-warning': successCommandPower == 'Pending',
                                    'text-bg-success': successCommandPower == 'Ok',
                                    'text-bg-secondary': successCommandPower == 'Unknown',
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

</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import * as bootstrap from 'bootstrap';
import {
    BIconXCircleFill,
    BIconExclamationCircleFill,
    BIconCheckCircleFill,
    BIconSpeedometer,
    BIconPower,
    BIconCpu,
    BIconJournalText,
    BIconToggleOn,
    BIconToggleOff,
    BIconArrowCounterclockwise
} from 'bootstrap-icons-vue';
import EventLog from '@/components/EventLog.vue';
import DevInfo from '@/components/DevInfo.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import InverterChannelInfo from "@/components/InverterChannelInfo.vue";
import InverterTotalInfo from '@/components/InverterTotalInfo.vue';
import type { DevInfoStatus } from '@/types/DevInfoStatus';
import type { EventlogItems } from '@/types/EventlogStatus';
import type { LiveData, Inverter } from '@/types/LiveDataStatus';
import type { LimitStatus } from '@/types/LimitStatus';
import type { LimitConfig } from '@/types/LimitConfig';
import { isLoggedIn, handleResponse, authHeader } from '@/utils/authentication';
import { formatNumber } from '@/utils';

export default defineComponent({
    components: {
        BasePage,
        InverterChannelInfo,
        InverterTotalInfo,
        EventLog,
        DevInfo,
        BootstrapAlert,
        BIconXCircleFill,
        BIconExclamationCircleFill,
        BIconCheckCircleFill,
        BIconSpeedometer,
        BIconPower,
        BIconCpu,
        BIconJournalText,
        BIconToggleOn,
        BIconToggleOff,
        BIconArrowCounterclockwise,
    },
    data() {
        return {
            isLogged: this.isLoggedIn(),

            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            liveData: {} as LiveData,
            isFirstFetchAfterConnect: true,
            eventLogView: {} as bootstrap.Modal,
            eventLogList: {} as EventlogItems,
            eventLogLoading: true,
            devInfoView: {} as bootstrap.Modal,
            devInfoList: {} as DevInfoStatus,
            devInfoLoading: true,

            limitSettingView: {} as bootstrap.Modal,
            limitSettingLoading: true,

            currentLimitList: {} as LimitStatus,
            targetLimitList: {} as LimitConfig,

            targetLimitMin: 2,
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
        this.$emitter.on("logged-in", () => {
            this.isLogged = this.isLoggedIn();
        });
        this.$emitter.on("logged-out", () => {
            this.isLogged = this.isLoggedIn();
        });
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

            const firstTabEl = document.querySelector(
                "#v-pills-tab:first-child button"
            );
            if (firstTabEl != null) {
                const firstTab = new bootstrap.Tab(firstTabEl);
                firstTab.show();
            }
        }
    },
    computed: {
        currentLimitAbsolute(): string {
            if (this.currentLimitList.max_power > 0) {
                return formatNumber(this.currentLimitList.limit_relative * this.currentLimitList.max_power / 100, 2);
            }
            return "0";
        },
        currentLimitRelative(): string {
            return formatNumber(this.currentLimitList.limit_relative, 2);
        },
        inverterData(): Inverter[] {
            return this.liveData.inverters;
        }
    },
    methods: {
        formatNumber,
        isLoggedIn,
        getInitialData() {
            this.dataLoading = true;
            fetch("/api/livedata/status")
                .then((response) => response.json())
                .then((data) => {
                    this.liveData = data;
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
                this.liveData = JSON.parse(event.data);
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
            this.showAlertLimit = false;
        },
        onShowLimitSettings(serial: number) {
            this.targetLimitList.serial = 0;
            this.targetLimitList.limit_value = 0;
            this.targetLimitType = 1;
            this.targetLimitTypeText = "Relative (%)";

            this.limitSettingLoading = true;
            fetch("/api/limit/status")
                .then((response) => response.json())
                .then((data) => {
                    this.currentLimitList = data[serial];
                    this.targetLimitList.serial = serial;
                    this.limitSettingLoading = false;
                });

            this.limitSettingView.show();
        },
        onSubmitLimit(e: Event) {
            e.preventDefault();

            this.targetLimitList.limit_type = (this.targetLimitPersistent ? 256 : 0) + this.targetLimitType
            const formData = new FormData();
            formData.append("data", JSON.stringify(this.targetLimitList));

            console.log(this.targetLimitList);

            fetch("/api/limit/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter))
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
                this.targetLimitMin = 2;
                this.targetLimitMax = 100;
            } else {
                this.targetLimitTypeText = "Absolute (W)";
                this.targetLimitMin = 10;
                this.targetLimitMax = (this.currentLimitList.max_power > 0 ? this.currentLimitList.max_power : 1500);
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
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter))
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