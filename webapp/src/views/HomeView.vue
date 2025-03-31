<template>
    <BasePage
        :title="$t('home.LiveData')"
        :isLoading="dataLoading"
        :isWideScreen="true"
        :showWebSocket="true"
        :isWebsocketConnected="isWebsocketConnected"
        @reload="reloadData"
    >
        <HintView :hints="liveData.hints" />
        <InverterTotalInfo :totalData="liveData.total" /><br />
        <div class="row gy-3">
            <div class="col-sm-3 col-md-2" :style="[inverterData.length == 1 ? { display: 'none' } : {}]">
                <div
                    class="nav nav-pills row-cols-sm-1 gap-3"
                    id="v-pills-tab"
                    role="tablist"
                    aria-orientation="vertical"
                >
                    <button
                        v-for="inverter in inverterData"
                        :key="inverter.serial"
                        class="nav-link border border-primary text-break"
                        :id="'v-pills-' + inverter.serial + '-tab'"
                        data-bs-toggle="pill"
                        :data-bs-target="'#v-pills-' + inverter.serial"
                        type="button"
                        role="tab"
                        aria-controls="'v-pills-' + inverter.serial"
                        aria-selected="true"
                    >
                        <div class="d-flex align-items-center">
                            <div class="me-2">
                                <span
                                    v-if="inverter.AC"
                                    class="badge"
                                    :class="{
                                        'text-bg-secondary': !inverter.poll_enabled,
                                        'text-bg-danger': inverter.poll_enabled && !inverter.reachable,
                                        'text-bg-warning':
                                            inverter.poll_enabled && inverter.reachable && !inverter.producing,
                                        'text-bg-success':
                                            inverter.poll_enabled && inverter.reachable && inverter.producing,
                                    }"
                                >
                                    {{ $n(inverter.AC[0]?.Power?.v || 0, 'decimalNoDigits') }}
                                    {{ inverter.AC[0].Power?.u }}
                                </span>
                                <span v-else class="badge text-bg-light">-</span>
                            </div>
                            <div class="ms-auto me-auto">
                                {{ inverter.name }}
                            </div>
                        </div>
                    </button>
                </div>
            </div>

            <div
                class="tab-content"
                id="v-pills-tabContent"
                :class="{
                    'col-sm-9 col-md-10': inverterData.length > 1,
                    'col-sm-12 col-md-12': inverterData.length == 1,
                }"
            >
                <div
                    v-for="inverter in inverterData"
                    :key="inverter.serial"
                    class="tab-pane fade show"
                    :id="'v-pills-' + inverter.serial"
                    role="tabpanel"
                    :aria-labelledby="'v-pills-' + inverter.serial + '-tab'"
                    tabindex="0"
                >
                    <div class="card">
                        <div
                            class="card-header d-flex justify-content-between align-items-center"
                            :class="{
                                'text-bg-tertiary': !inverter.poll_enabled,
                                'text-bg-danger': inverter.poll_enabled && !inverter.reachable,
                                'text-bg-warning': inverter.poll_enabled && inverter.reachable && !inverter.producing,
                                'text-bg-success': inverter.poll_enabled && inverter.reachable && inverter.producing,
                            }"
                        >
                            <div class="p-1 flex-grow-1">
                                <div class="d-flex flex-wrap">
                                    <div style="padding-right: 2em">
                                        {{ inverter.name }}
                                    </div>
                                    <div style="padding-right: 2em">
                                        {{ $t('home.SerialNumber') }}{{ inverter.serial }}
                                    </div>
                                    <div style="padding-right: 2em">
                                        {{ $t('home.CurrentLimit') }}:
                                        <template v-if="inverter.limit_absolute > -1">
                                            {{ $n(inverter.limit_absolute, 'decimalNoDigits') }} W | </template
                                        >{{ $n(inverter.limit_relative / 100, 'percentOneDigit') }}
                                    </div>
                                    <div style="padding-right: 2em">
                                        <DataAgeDisplay :data-age-ms="inverter.data_age_ms" />
                                    </div>
                                </div>
                            </div>
                            <div class="btn-toolbar p-2" role="toolbar">
                                <div class="btn-group me-2" role="group">
                                    <button
                                        :disabled="!isLogged"
                                        type="button"
                                        class="btn btn-sm btn-danger"
                                        @click="onShowLimitSettings(inverter.serial)"
                                        v-tooltip
                                        :title="$t('home.ShowSetInverterLimit')"
                                    >
                                        <BIconSpeedometer style="font-size: 24px" />
                                    </button>
                                </div>

                                <div class="btn-group me-2" role="group">
                                    <button
                                        :disabled="!isLogged"
                                        type="button"
                                        class="btn btn-sm btn-danger"
                                        @click="onShowPowerSettings(inverter.serial)"
                                        v-tooltip
                                        :title="$t('home.TurnOnOff')"
                                    >
                                        <BIconPower style="font-size: 24px" />
                                    </button>
                                </div>

                                <div class="btn-group me-2" role="group">
                                    <button
                                        type="button"
                                        class="btn btn-sm btn-info"
                                        @click="onShowDevInfo(inverter.serial)"
                                        v-tooltip
                                        :title="$t('home.ShowInverterInfo')"
                                    >
                                        <BIconCpu style="font-size: 24px" />
                                    </button>
                                </div>

                                <div class="btn-group me-2" role="group">
                                    <button
                                        type="button"
                                        class="btn btn-sm btn-info"
                                        @click="onShowGridProfile(inverter.serial)"
                                        v-tooltip
                                        :title="$t('home.ShowGridProfile')"
                                    >
                                        <BIconOutlet style="font-size: 24px" />
                                    </button>
                                </div>

                                <div class="btn-group" role="group">
                                    <button
                                        v-if="inverter.events >= 0"
                                        type="button"
                                        class="btn btn-sm btn-secondary position-relative"
                                        @click="onShowEventlog(inverter.serial)"
                                        v-tooltip
                                        :title="$t('home.ShowEventlog')"
                                    >
                                        <BIconJournalText style="font-size: 24px" />
                                        <span
                                            class="position-absolute top-0 start-100 translate-middle badge rounded-pill text-bg-danger"
                                        >
                                            {{ inverter.events }}
                                            <span class="visually-hidden">{{ $t('home.UnreadMessages') }}</span>
                                        </span>
                                    </button>
                                </div>
                            </div>
                        </div>
                        <div class="card-body">
                            <div class="row flex-row-reverse flex-wrap-reverse g-3">
                                <template
                                    v-for="chanType in [
                                        { obj: inverter.INV, name: 'INV' },
                                        { obj: inverter.AC, name: 'AC' },
                                        { obj: inverter.DC, name: 'DC' },
                                    ].reverse()"
                                >
                                    <template v-if="chanType.obj != null">
                                        <template
                                            v-for="channel in Object.keys(chanType.obj)
                                                .sort()
                                                .reverse()
                                                .map((x) => +x)"
                                            :key="channel"
                                        >
                                            <template
                                                v-if="
                                                    chanType.name != 'DC' ||
                                                    (chanType.name == 'DC' && getSumIrridiation(inverter) == 0) ||
                                                    (chanType.name == 'DC' &&
                                                        getSumIrridiation(inverter) > 0 &&
                                                        chanType.obj[channel].Irradiation?.max) ||
                                                    0 > 0
                                                "
                                            >
                                                <div class="col">
                                                    <InverterChannelInfo
                                                        :channelData="chanType.obj[channel]"
                                                        :channelType="chanType.name"
                                                        :channelNumber="channel"
                                                    />
                                                </div>
                                            </template>
                                        </template>
                                    </template>
                                </template>
                            </div>

                            <BootstrapAlert class="m-3" :show="!inverter.hasOwnProperty('INV')">
                                <div class="d-flex justify-content-center align-items-center">
                                    <div class="spinner-border m-1" role="status">
                                        <span class="visually-hidden">{{ $t('home.LoadingInverter') }}</span>
                                    </div>
                                    <span>{{ $t('home.LoadingInverter') }}</span>
                                </div>
                            </BootstrapAlert>

                            <div class="accordion mt-5" id="accordionRadioStats">
                                <div class="accordion-item accordion-table">
                                    <h2 class="accordion-header">
                                        <button
                                            class="accordion-button collapsed"
                                            type="button"
                                            data-bs-toggle="collapse"
                                            data-bs-target="#collapseStats"
                                            aria-expanded="true"
                                            aria-controls="collapseStats"
                                        >
                                            <BIconBroadcast />&nbsp;{{ $t('home.RadioStats') }}
                                        </button>
                                    </h2>
                                    <div
                                        id="collapseStats"
                                        class="accordion-collapse collapse"
                                        data-bs-parent="#accordionRadioStats"
                                    >
                                        <div class="accordion-body">
                                            <table class="table table-striped table-hover">
                                                <tbody>
                                                    <tr>
                                                        <td>{{ $t('home.TxRequest') }}</td>
                                                        <td>{{ $n(inverter.radio_stats.tx_request) }}</td>
                                                        <td></td>
                                                    </tr>
                                                    <tr>
                                                        <td>{{ $t('home.RxSuccess') }}</td>
                                                        <td>{{ $n(inverter.radio_stats.rx_success) }}</td>
                                                        <td>
                                                            {{
                                                                ratio(
                                                                    inverter.radio_stats.rx_success,
                                                                    inverter.radio_stats.tx_request
                                                                )
                                                            }}
                                                        </td>
                                                    </tr>
                                                    <tr>
                                                        <td>{{ $t('home.RxFailNothing') }}</td>
                                                        <td>{{ $n(inverter.radio_stats.rx_fail_nothing) }}</td>
                                                        <td>
                                                            {{
                                                                ratio(
                                                                    inverter.radio_stats.rx_fail_nothing,
                                                                    inverter.radio_stats.tx_request
                                                                )
                                                            }}
                                                        </td>
                                                    </tr>
                                                    <tr>
                                                        <td>{{ $t('home.RxFailPartial') }}</td>
                                                        <td>{{ $n(inverter.radio_stats.rx_fail_partial) }}</td>
                                                        <td>
                                                            {{
                                                                ratio(
                                                                    inverter.radio_stats.rx_fail_partial,
                                                                    inverter.radio_stats.tx_request
                                                                )
                                                            }}
                                                        </td>
                                                    </tr>
                                                    <tr>
                                                        <td>{{ $t('home.RxFailCorrupt') }}</td>
                                                        <td>{{ $n(inverter.radio_stats.rx_fail_corrupt) }}</td>
                                                        <td>
                                                            {{
                                                                ratio(
                                                                    inverter.radio_stats.rx_fail_corrupt,
                                                                    inverter.radio_stats.tx_request
                                                                )
                                                            }}
                                                        </td>
                                                    </tr>
                                                    <tr>
                                                        <td>{{ $t('home.TxReRequest') }}</td>
                                                        <td>{{ $n(inverter.radio_stats.tx_re_request) }}</td>
                                                        <td></td>
                                                    </tr>
                                                    <tr>
                                                        <td>
                                                            {{ $t('home.Rssi') }}
                                                            <BIconInfoCircle v-tooltip :title="$t('home.RssiHint')" />
                                                        </td>
                                                        <td>
                                                            {{ $t('home.dBm', { dbm: $n(inverter.radio_stats.rssi) }) }}
                                                        </td>
                                                        <td></td>
                                                    </tr>
                                                </tbody>
                                            </table>
                                            <div class="d-flex">
                                                <button
                                                    :disabled="!isLogged || performRadioStatsReset"
                                                    type="button"
                                                    class="btn btn-danger ms-auto me-3 mt-3"
                                                    @click="onResetRadioStats(inverter.serial)"
                                                >
                                                    <template v-if="!performRadioStatsReset">
                                                        <BIconArrowCounterclockwise />&nbsp;{{ $t('home.StatsReset') }}
                                                    </template>
                                                    <template v-else>
                                                        <span
                                                            class="spinner-border spinner-border-sm"
                                                            aria-hidden="true"
                                                        ></span>
                                                        <span role="status">&nbsp;{{ $t('home.StatsResetting') }}</span>
                                                    </template>
                                                </button>
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </BasePage>

    <ModalDialog modalId="eventView" :title="$t('home.EventLog')" :loading="eventLogLoading">
        <EventLog :eventLogList="eventLogList" />
    </ModalDialog>

    <ModalDialog modalId="devInfoView" :title="$t('home.InverterInfo')" :loading="devInfoLoading">
        <DevInfo :devInfoList="devInfoList" />
    </ModalDialog>

    <ModalDialog modalId="gridProfileView" :title="$t('home.GridProfile')" :loading="gridProfileLoading">
        <GridProfile :gridProfileList="gridProfileList" :gridProfileRawList="gridProfileRawList" />
    </ModalDialog>

    <ModalDialog modalId="limitSettingView" :title="$t('home.LimitSettings')" :loading="limitSettingLoading">
        <BootstrapAlert v-model="showAlertLimit" :variant="alertTypeLimit">
            {{ alertMessageLimit }}
        </BootstrapAlert>

        <div class="row mb-3">
            <label for="inputCurrentLimit" class="col-sm-3 col-form-label">{{ $t('home.CurrentLimit') }} </label>
            <div class="col-sm-4">
                <div class="input-group">
                    <input
                        type="text"
                        class="form-control"
                        id="inputCurrentLimit"
                        aria-describedby="currentLimitType"
                        v-model="currentLimitRelative"
                        disabled
                    />
                    <span class="input-group-text" id="currentLimitType">%</span>
                </div>
            </div>

            <div class="col-sm-4" v-if="currentLimitList.max_power > 0">
                <div class="input-group">
                    <input
                        type="text"
                        class="form-control"
                        id="inputCurrentLimitAbsolute"
                        aria-describedby="currentLimitTypeAbsolute"
                        v-model="currentLimitAbsolute"
                        disabled
                    />
                    <span class="input-group-text" id="currentLimitTypeAbsolute">W</span>
                </div>
            </div>
        </div>

        <div class="row mb-3 align-items-center">
            <label for="inputLastLimitSet" class="col-sm-3 col-form-label">
                {{ $t('home.LastLimitSetStatus') }}
            </label>
            <div class="col-sm-9">
                <span
                    class="badge"
                    :class="{
                        'text-bg-danger': currentLimitList.limit_set_status == 'Failure',
                        'text-bg-warning': currentLimitList.limit_set_status == 'Pending',
                        'text-bg-success': currentLimitList.limit_set_status == 'Ok',
                        'text-bg-secondary': currentLimitList.limit_set_status == 'Unknown',
                    }"
                >
                    {{ $t('home.' + currentLimitList.limit_set_status) }}
                </span>
            </div>
        </div>

        <div class="row mb-3">
            <label for="inputTargetLimit" class="col-sm-3 col-form-label">{{ $t('home.SetLimit') }}</label>
            <div class="col-sm-9">
                <div class="input-group">
                    <input
                        type="number"
                        name="inputTargetLimit"
                        class="form-control"
                        id="inputTargetLimit"
                        :min="targetLimitMin"
                        :max="targetLimitMax"
                        v-model="targetLimitList.limit_value"
                    />
                    <button
                        class="btn btn-primary dropdown-toggle"
                        type="button"
                        data-bs-toggle="dropdown"
                        aria-expanded="false"
                    >
                        {{ targetLimitTypeText }}
                    </button>
                    <ul class="dropdown-menu dropdown-menu-end">
                        <li>
                            <a class="dropdown-item" @click="onSelectType(1)" href="#">{{ $t('home.Relative') }}</a>
                        </li>
                        <li>
                            <a class="dropdown-item" @click="onSelectType(0)" href="#">{{ $t('home.Absolute') }}</a>
                        </li>
                    </ul>
                </div>
                <div
                    v-if="targetLimitType == 0"
                    class="alert alert-secondary mt-3"
                    role="alert"
                    v-html="$t('home.LimitHint')"
                ></div>
            </div>
        </div>

        <template #footer>
            <button type="button" class="btn btn-danger" @click="onSetLimitSettings(true)">
                {{ $t('home.SetPersistent') }}
            </button>

            <button type="button" class="btn btn-danger" @click="onSetLimitSettings(false)">
                {{ $t('home.SetNonPersistent') }}
            </button>
        </template>
    </ModalDialog>

    <ModalDialog modalId="powerSettingView" :title="$t('home.PowerSettings')" :loading="powerSettingLoading">
        <BootstrapAlert v-model="showAlertPower" :variant="alertTypePower">
            {{ alertMessagePower }}
        </BootstrapAlert>

        <div class="row mb-3 align-items-center">
            <label for="inputLastPowerSet" class="col col-form-label">{{ $t('home.LastPowerSetStatus') }}</label>
            <div class="col">
                <span
                    class="badge"
                    :class="{
                        'text-bg-danger': successCommandPower == 'Failure',
                        'text-bg-warning': successCommandPower == 'Pending',
                        'text-bg-success': successCommandPower == 'Ok',
                        'text-bg-secondary': successCommandPower == 'Unknown',
                    }"
                >
                    {{ $t('home.' + successCommandPower) }}
                </span>
            </div>
        </div>

        <div class="d-grid gap-2 col-6 mx-auto">
            <button type="button" class="btn btn-success" @click="onSetPowerSettings(true)">
                <BIconToggleOn class="fs-4" />&nbsp;{{ $t('home.TurnOn') }}
            </button>
            <button type="button" class="btn btn-danger" @click="onSetPowerSettings(false)">
                <BIconToggleOff class="fs-4" />&nbsp;{{ $t('home.TurnOff') }}
            </button>
            <button type="button" class="btn btn-warning" @click="onSetPowerSettings(true, true)">
                <BIconArrowCounterclockwise class="fs-4" />&nbsp;{{ $t('home.Restart') }}
            </button>
        </div>
    </ModalDialog>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import DataAgeDisplay from '@/components/DataAgeDisplay.vue';
import DevInfo from '@/components/DevInfo.vue';
import EventLog from '@/components/EventLog.vue';
import GridProfile from '@/components/GridProfile.vue';
import HintView from '@/components/HintView.vue';
import InverterChannelInfo from '@/components/InverterChannelInfo.vue';
import InverterTotalInfo from '@/components/InverterTotalInfo.vue';
import ModalDialog from '@/components/ModalDialog.vue';
import type { DevInfoStatus } from '@/types/DevInfoStatus';
import type { EventlogItems } from '@/types/EventlogStatus';
import type { GridProfileRawdata } from '@/types/GridProfileRawdata';
import type { GridProfileStatus } from '@/types/GridProfileStatus';
import type { LimitConfig } from '@/types/LimitConfig';
import type { LimitStatus } from '@/types/LimitStatus';
import type { Inverter, LiveData } from '@/types/LiveDataStatus';
import { authHeader, authUrl, handleResponse, isLoggedIn } from '@/utils/authentication';
import * as bootstrap from 'bootstrap';
import {
    BIconArrowCounterclockwise,
    BIconBroadcast,
    BIconCpu,
    BIconInfoCircle,
    BIconJournalText,
    BIconOutlet,
    BIconPower,
    BIconSpeedometer,
    BIconToggleOff,
    BIconToggleOn,
} from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        DataAgeDisplay,
        DevInfo,
        EventLog,
        GridProfile,
        HintView,
        InverterChannelInfo,
        InverterTotalInfo,
        ModalDialog,
        BIconArrowCounterclockwise,
        BIconBroadcast,
        BIconCpu,
        BIconInfoCircle,
        BIconJournalText,
        BIconOutlet,
        BIconPower,
        BIconSpeedometer,
        BIconToggleOff,
        BIconToggleOn,
    },
    data() {
        return {
            isLogged: this.isLoggedIn(),

            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeTimers: {} as Record<string, number>,
            dataLoading: true,
            liveData: {} as LiveData,
            isFirstFetchAfterConnect: true,
            eventLogView: {} as bootstrap.Modal,
            eventLogList: {} as EventlogItems,
            eventLogLoading: true,
            devInfoView: {} as bootstrap.Modal,
            devInfoList: {} as DevInfoStatus,
            devInfoLoading: true,
            gridProfileView: {} as bootstrap.Modal,
            gridProfileList: {} as GridProfileStatus,
            gridProfileRawList: {} as GridProfileRawdata,
            gridProfileLoading: true,

            limitSettingView: {} as bootstrap.Modal,
            limitSettingLoading: true,

            currentLimitList: {} as LimitStatus,
            targetLimitList: {} as LimitConfig,

            targetLimitMin: 0,
            targetLimitMax: 100,
            targetLimitTypeText: this.$t('home.Relative'),
            targetLimitType: 1,

            alertMessageLimit: '',
            alertTypeLimit: 'info',
            showAlertLimit: false,
            performRadioStatsReset: false,

            powerSettingView: {} as bootstrap.Modal,
            powerSettingSerial: '',
            powerSettingLoading: true,
            alertMessagePower: '',
            alertTypePower: 'info',
            showAlertPower: false,
            successCommandPower: '',

            isWebsocketConnected: false,
        };
    },
    created() {
        this.getInitialData();
        this.initSocket();
        this.$emitter.on('logged-in', () => {
            this.isLogged = this.isLoggedIn();
        });
        this.$emitter.on('logged-out', () => {
            this.isLogged = this.isLoggedIn();
        });
    },
    mounted() {
        this.eventLogView = new bootstrap.Modal('#eventView');
        this.devInfoView = new bootstrap.Modal('#devInfoView');
        this.gridProfileView = new bootstrap.Modal('#gridProfileView');
        this.limitSettingView = new bootstrap.Modal('#limitSettingView');
        this.powerSettingView = new bootstrap.Modal('#powerSettingView');
    },
    unmounted() {
        this.closeSocket();
    },
    updated() {
        console.log('Updated');
        // Select first tab
        if (this.isFirstFetchAfterConnect) {
            console.log('isFirstFetchAfterConnect');

            this.$nextTick(() => {
                console.log('nextTick');
                const firstTabEl = document.querySelector('#v-pills-tab:first-child button');
                if (firstTabEl != null) {
                    this.isFirstFetchAfterConnect = false;
                    console.log('Show');
                    const firstTab = new bootstrap.Tab(firstTabEl);
                    firstTab.show();
                }
            });
        }
    },
    computed: {
        currentLimitAbsolute(): string {
            if (this.currentLimitList.max_power > 0) {
                return this.$n(
                    (this.currentLimitList.limit_relative * this.currentLimitList.max_power) / 100,
                    'decimalNoDigits'
                );
            }
            return '0';
        },
        currentLimitRelative(): string {
            return this.$n(this.currentLimitList.limit_relative, 'decimalOneDigit');
        },
        inverterData(): Inverter[] {
            return this.liveData.inverters.slice().sort((a: Inverter, b: Inverter) => {
                return a.order - b.order;
            });
        },
    },
    methods: {
        isLoggedIn,
        getInitialData(triggerLoading: boolean = true) {
            if (triggerLoading) {
                this.dataLoading = true;
            }
            fetch('/api/livedata/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.liveData = data;
                    if (triggerLoading) {
                        this.dataLoading = false;
                    }
                });
        },
        reloadData() {
            this.closeSocket();

            setTimeout(() => {
                this.getInitialData(false);
                this.initSocket();
            }, 1000);
        },
        initSocket() {
            console.log('Starting connection to WebSocket Server');

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/livedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);
                if (event.data != '{}') {
                    const newData = JSON.parse(event.data);
                    Object.assign(this.liveData.total, newData.total);
                    Object.assign(this.liveData.hints, newData.hints);

                    const foundIdx = this.liveData.inverters.findIndex(
                        (element) => element.serial == newData.inverters[0].serial
                    );
                    if (foundIdx == -1) {
                        Object.assign(this.liveData.inverters, newData.inverters);
                        this.liveData.inverters.forEach((inv) => this.resetDataAging(inv));
                    } else {
                        Object.assign(this.liveData.inverters[foundIdx], newData.inverters[0]);
                        this.resetDataAging(this.liveData.inverters[foundIdx]);
                    }
                    this.dataLoading = false;
                    this.heartCheck(); // Reset heartbeat detection
                } else {
                    // Sometimes it does not recover automatically so have to force a reconnect
                    this.closeSocket();
                    this.heartCheck(10); // Reconnect faster
                }
            };

            this.socket.onopen = (event) => {
                console.log(event);
                console.log('Successfully connected to the echo websocket server...');
                this.isWebsocketConnected = true;
            };

            this.socket.onclose = () => {
                console.log('Connection to websocket closed...');
                this.isWebsocketConnected = false;
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        resetDataAging(inv: Inverter) {
            if (this.dataAgeTimers[inv.serial] !== undefined) {
                clearTimeout(this.dataAgeTimers[inv.serial]);
            }

            const nextMs = 1000 - (inv.data_age_ms % 1000);
            this.dataAgeTimers[inv.serial] = setTimeout(() => {
                this.doDataAging(inv.serial);
            }, nextMs);
        },
        doDataAging(serial: string) {
            const inv = this.liveData?.inverters?.find((inv) => inv.serial === serial);
            if (inv === undefined) {
                return;
            }

            inv.data_age_ms += 1000;

            this.dataAgeTimers[serial] = setTimeout(() => {
                this.doDataAging(serial);
            }, 1000);
        },
        // Send heartbeat packets regularly * 59s Send a heartbeat
        heartCheck(duration: number = 59) {
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
            }, duration * 1000);
        },
        /** To break off websocket Connect */
        closeSocket() {
            this.socket.close();
            if (this.heartInterval) {
                clearTimeout(this.heartInterval);
            }
            this.isFirstFetchAfterConnect = true;
        },
        onShowEventlog(serial: string) {
            this.eventLogLoading = true;
            fetch('/api/eventlog/status?inv=' + serial + '&locale=' + this.$i18n.locale, {
                headers: authHeader(),
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.eventLogList = data;
                    this.eventLogLoading = false;
                });

            this.eventLogView.show();
        },
        onShowDevInfo(serial: string) {
            this.devInfoLoading = true;
            fetch('/api/devinfo/status?inv=' + serial, { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.devInfoList = data;
                    this.devInfoList.serial = serial;
                    this.devInfoLoading = false;
                });

            this.devInfoView.show();
        },
        onShowGridProfile(serial: string) {
            this.gridProfileLoading = true;
            fetch('/api/gridprofile/status?inv=' + serial, { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.gridProfileList = data;

                    fetch('/api/gridprofile/rawdata?inv=' + serial, { headers: authHeader() })
                        .then((response) => handleResponse(response, this.$emitter, this.$router))
                        .then((data) => {
                            this.gridProfileRawList = data;
                            this.gridProfileLoading = false;
                        });
                });

            this.gridProfileView.show();
        },
        onShowLimitSettings(serial: string) {
            this.showAlertLimit = false;
            this.targetLimitList.serial = '';
            this.targetLimitList.limit_value = 0;
            this.targetLimitType = 1;
            this.targetLimitTypeText = this.$t('home.Relative');

            this.limitSettingLoading = true;
            fetch('/api/limit/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.currentLimitList = data[serial];
                    this.targetLimitList.serial = serial;
                    this.limitSettingLoading = false;
                });

            this.limitSettingView.show();
        },
        onResetRadioStats(serial: string) {
            this.performRadioStatsReset = true;
            fetch('/api/inverter/stats_reset?inv=' + serial, { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(() => {
                    this.performRadioStatsReset = false;
                });
        },
        onSetLimitSettings(setPersistent: boolean) {
            this.targetLimitList.limit_type = (setPersistent ? 256 : 0) + this.targetLimitType;
            const formData = new FormData();
            formData.append('data', JSON.stringify(this.targetLimitList));

            console.log(this.targetLimitList);

            fetch('/api/limit/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    if (response.type == 'success') {
                        this.limitSettingView.hide();
                    } else {
                        this.alertMessageLimit = this.$t('apiresponse.' + response.code, response.param);
                        this.alertTypeLimit = response.type;
                        this.showAlertLimit = true;
                    }
                });
        },
        onSelectType(type: number) {
            if (type == 1) {
                this.targetLimitTypeText = this.$t('home.Relative');
                this.targetLimitMin = 0;
                this.targetLimitMax = 100;
            } else {
                this.targetLimitTypeText = this.$t('home.Absolute');
                this.targetLimitMin = 0;
                this.targetLimitMax = this.currentLimitList.max_power > 0 ? this.currentLimitList.max_power : 2250;
            }
            this.targetLimitType = type;
        },

        onShowPowerSettings(serial: string) {
            this.showAlertPower = false;
            this.powerSettingSerial = '';
            this.powerSettingLoading = true;
            fetch('/api/power/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.successCommandPower = data[serial].power_set_status;
                    this.powerSettingSerial = serial;
                    this.powerSettingLoading = false;
                });
            this.powerSettingView.show();
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
            formData.append('data', JSON.stringify(data));

            console.log(data);

            fetch('/api/power/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    if (response.type == 'success') {
                        this.powerSettingView.hide();
                    } else {
                        this.alertMessagePower = this.$t('apiresponse.' + response.code, response.param);
                        this.alertTypePower = response.type;
                        this.showAlertPower = true;
                    }
                });
        },
        getSumIrridiation(inv: Inverter): number {
            let total = 0;
            Object.keys(inv.DC).forEach((key) => {
                total += inv.DC[key as unknown as number].Irradiation?.max || 0;
            });
            return total;
        },
        ratio(val_small: number, val_large: number): string {
            if (val_large == 0) {
                return '-';
            }
            return this.$n(val_small / val_large, 'percent');
        },
    },
});
</script>

<style>
.btn-group {
    border-radius: var(--bs-border-radius);
    margin-top: 0.25rem;
}
</style>
