<template>
    <div class="text-center" v-if="dataLoading">
        <div class="spinner-border" role="status">
            <span class="visually-hidden">Loading...</span>
        </div>
    </div>

    <template v-else>
        <div class="row gy-3 mt-0" v-for="(item, serial) in solarcharger.instances" :key="serial">
            <div class="tab-content col-sm-12 col-md-12" id="v-pills-tabContent">
                <div class="card">
                    <div
                        class="card-header d-flex justify-content-between align-items-center"
                        :class="{
                            'text-bg-danger': item.data_age_ms >= 10000,
                            'text-bg-success': item.data_age_ms < 10000,
                        }"
                    >
                        <div class="p-1 flex-grow-1">
                            <div class="d-flex flex-wrap">
                                <div style="padding-right: 2em">
                                    <template v-if="item.product_id === 'MQTT'">
                                        {{ $t('solarchargerhome.MqttProduct') }}
                                    </template>
                                    <template v-else>
                                        {{ item.product_id }}
                                    </template>
                                </div>
                                <div v-if="item.hide_serial !== true" style="padding-right: 2em">
                                    {{ $t('solarchargerhome.SerialNumber') }}: {{ serial }}
                                </div>
                                <div v-if="item.firmware_version" style="padding-right: 2em">
                                    {{ $t('solarchargerhome.FirmwareVersion') }}: {{ item.firmware_version }}
                                </div>
                                <DataAgeDisplay :data-age-ms="item.data_age_ms" />
                            </div>
                        </div>
                        <div class="btn-group me-2" role="group">
                            <button
                                type="button"
                                class="btn btn-sm"
                                v-tooltip
                                :title="$t('solarchargerhome.PowerLimiterState')"
                            >
                                <div v-if="dplData.PLSTATE == 0">
                                    <BIconXCircleFill style="font-size: 24px" />
                                </div>
                                <div v-else-if="dplData.PLSTATE == 1">
                                    <BIconBatteryCharging style="font-size: 24px" />
                                </div>
                                <div v-else-if="dplData.PLSTATE == 2">
                                    <BIconSun style="font-size: 24px" />
                                </div>
                                <div v-else-if="dplData.PLSTATE == 3">
                                    <BIconBatteryHalf style="font-size: 24px" />
                                </div>
                                <span
                                    v-if="dplData.PLSTATE != -1"
                                    class="position-absolute top-0 start-100 translate-middle badge rounded-pill text-bg-info"
                                >
                                    {{ dplData.PLLIMIT }} W
                                </span>
                            </button>
                        </div>
                    </div>
                    <div class="card-body">
                        <div class="row flex-row flex-wrap align-items-start g-3">
                            <div v-for="(values, section) in item.values" v-bind:key="section" class="col order-0">
                                <div class="card card-table" :class="{ 'border-info': section === 'device' }">
                                    <div :class="section === 'device' ? 'card-header text-bg-info' : 'card-header'">
                                        {{ $t('solarchargerhome.section_' + section) }}
                                    </div>
                                    <div class="card-body">
                                        <div class="table-responsive">
                                            <table class="table table-striped table-hover">
                                                <thead>
                                                    <tr>
                                                        <th scope="col">{{ $t('solarchargerhome.Property') }}</th>
                                                        <th class="value" scope="col">
                                                            {{ $t('solarchargerhome.Value') }}
                                                        </th>
                                                        <th scope="col">{{ $t('solarchargerhome.Unit') }}</th>
                                                    </tr>
                                                </thead>
                                                <tbody>
                                                    <tr v-for="(prop, key) in values" v-bind:key="key">
                                                        <th scope="row">
                                                            {{ $t('solarchargerhome.' + section + '.' + key) }}
                                                        </th>
                                                        <td class="value">
                                                            <template v-if="typeof prop === 'string'">
                                                                {{ prop }}
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
                                                        <td v-if="typeof prop === 'string'"></td>
                                                        <td v-else>{{ prop.u }}</td>
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
import type { DynamicPowerLimiter, SolarCharger } from '@/types/SolarChargerLiveDataStatus';
import { handleResponse, authHeader, authUrl } from '@/utils/authentication';
import { BIconSun, BIconBatteryCharging, BIconBatteryHalf, BIconXCircleFill } from 'bootstrap-icons-vue';
import DataAgeDisplay from '@/components/DataAgeDisplay.vue';

export default defineComponent({
    components: {
        BIconSun,
        BIconBatteryCharging,
        BIconBatteryHalf,
        BIconXCircleFill,
        DataAgeDisplay,
    },
    data() {
        return {
            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeTimers: {} as Record<string, number>,
            dataLoading: true,
            dplData: {} as DynamicPowerLimiter,
            solarcharger: {} as SolarCharger,
            isFirstFetchAfterConnect: true,
        };
    },
    created() {
        this.getInitialData();
        this.initSocket();
    },
    unmounted() {
        this.closeSocket();
    },
    methods: {
        getInitialData() {
            console.log('Get initalData for SolarChargerView');
            this.dataLoading = true;
            fetch('/api/solarchargerlivedata/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((root) => {
                    this.dplData = root['dpl'];
                    this.solarcharger = root['solarcharger'];
                    this.dataLoading = false;
                    this.resetDataAging(Object.keys(root['solarcharger']['instances']));
                });
        },
        initSocket() {
            console.log('Starting connection to SolarCharger WebSocket Server');

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/solarchargerlivedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                console.log(event);
                const root = JSON.parse(event.data);
                this.dplData = root['dpl'];
                if (root['solarcharger']['full_update'] === true) {
                    this.solarcharger = root['solarcharger'];
                } else {
                    Object.assign(this.solarcharger.instances, root['solarcharger']['instances']);
                }
                this.resetDataAging(Object.keys(root['solarcharger']['instances']));
                this.dataLoading = false;
                this.heartCheck(); // Reset heartbeat detection
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log('Successfully connected to the SolarCharger websocket server...');
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        resetDataAging(serials: Array<string>) {
            serials.forEach((serial) => {
                if (this.dataAgeTimers[serial] !== undefined) {
                    clearTimeout(this.dataAgeTimers[serial]);
                }

                const nextMs = 1000 - ((this.solarcharger?.instances?.[serial]?.data_age_ms ?? 0) % 1000);
                this.dataAgeTimers[serial] = setTimeout(() => {
                    this.doDataAging(serial);
                }, nextMs);
            });
        },
        doDataAging(serial: string) {
            if (this.solarcharger?.instances?.[serial] === undefined) {
                return;
            }

            this.solarcharger.instances[serial].data_age_ms += 1000;

            this.dataAgeTimers[serial] = setTimeout(() => {
                this.doDataAging(serial);
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
});
</script>
