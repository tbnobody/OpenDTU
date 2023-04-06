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
          <div class="card-header d-flex justify-content-between align-items-center" :class="{
            'text-bg-danger': batteryData.data_age > 20,
            'text-bg-primary': batteryData.data_age < 20,
          }">
            <div class="p-1 flex-grow-1">
              <div class="d-flex flex-wrap">
                <div style="padding-right: 2em;">
                  {{ $t('battery.battery') }}: {{ batteryData.manufacturer }}
                </div>
                <div style="padding-right: 2em;">
                  {{ $t('battery.DataAge') }} {{ $t('battery.Seconds', { 'val': batteryData.data_age }) }}
                </div>
              </div>
            </div>
          </div>

          <div class="card-body">
            <div class="row flex-row flex-wrap align-items-start g-3">
              <div class="col order-0">
                <div class="card" :class="{ 'border-info': true }">
                  <div class="card-header bg-info">{{ $t('battery.Status') }}</div>
                  <div class="card-body">
                    <table class="table table-striped table-hover">
                      <thead>
                        <tr>
                          <th scope="col">{{ $t('battery.Property') }}</th>
                          <th style="text-align: right" scope="col">{{ $t('battery.Value') }}</th>
                          <th scope="col">{{ $t('battery.Unit') }}</th>
                        </tr>
                      </thead>
                      <tbody>
                        <tr>
                          <th scope="row">{{ $t('battery.stateOfCharge') }}</th>
                          <td style="text-align: right">{{ batteryData.stateOfCharge.v.toFixed(1) }}</td>
                          <td>{{ batteryData.stateOfCharge.u }}</td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.stateOfHealth') }}</th>
                          <td style="text-align: right">{{ batteryData.stateOfHealth.v.toFixed(1) }}</td>
                          <td>{{ batteryData.stateOfHealth.u }}</td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.voltage') }}</th>
                          <td style="text-align: right">{{ batteryData.voltage.v.toFixed(1) }}</td>
                          <td>{{ batteryData.voltage.u }}</td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.current') }}</th>
                          <td style="text-align: right">{{ batteryData.current.v.toFixed(1) }}</td>
                          <td>{{ batteryData.current.u }}</td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.temperature') }}</th>
                          <td style="text-align: right">{{ batteryData.temperature.v.toFixed(1) }}</td>
                          <td>{{ batteryData.temperature.u }}</td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.chargeVoltage') }}</th>
                          <td style="text-align: right">{{ batteryData.chargeVoltage.v.toFixed(1) }}</td>
                          <td>{{ batteryData.chargeVoltage.u }}</td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.chargeCurrentLimitation') }}</th>
                          <td style="text-align: right">{{ batteryData.chargeCurrentLimitation.v }}</td>
                          <td>{{ batteryData.chargeCurrentLimitation.u }}</td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.dischargeCurrentLimitation') }}</th>
                          <td style="text-align: right">{{ batteryData.dischargeCurrentLimitation.v }}</td>
                          <td>{{ batteryData.dischargeCurrentLimitation.u }}</td>
                        </tr> 
                      </tbody>
                    </table>
                  </div>
                </div>
              </div>
              <div class="col order-1">
                <div class="card" :class="{ 'border-info': false }">
                  <div class="card-header bg-info">{{ $t('battery.warn_alarm') }}</div>
                  <div class="card-body">
                    <table class="table table-striped table-hover">
                      <thead>
                        <tr>
                          <th scope="col">{{ $t('battery.Property') }}</th>
                          <th scope="col">{{ $t('battery.alarm') }}</th>
                          <th scope="col">{{ $t('battery.warning') }}</th>
                        </tr>
                      </thead>
                      <tbody>
                        <tr>
                          <th scope="row">{{ $t('battery.dischargeCurrent') }}</th>
                          <td>
                            <span class="badge" :class="{
                                'text-bg-danger': batteryData.alarms.dischargeCurrent,
                                'text-bg-success': !batteryData.alarms.dischargeCurrent
                            }">
                                <template v-if="!batteryData.alarms.dischargeCurrent">{{ $t('battery.ok') }}</template>
                                <template v-else>{{ $t('battery.alarm') }}</template>
                            </span>
                          </td>
                          <td>
                              <span class="badge" :class="{
                                  'text-bg-warning text-dark': batteryData.warnings.dischargeCurrent,
                                  'text-bg-success': !batteryData.warnings.dischargeCurrent
                              }">
                                  <template v-if="!batteryData.warnings.dischargeCurrent">{{ $t('battery.ok') }}</template>
                                  <template v-else>{{ $t('battery.warning') }}</template>
                              </span>
                          </td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.chargeCurrent') }}</th>
                          <td>
                            <span class="badge" :class="{
                                'text-bg-danger': batteryData.alarms.chargeCurrent,
                                'text-bg-success': !batteryData.alarms.chargeCurrent
                            }">
                                <template v-if="!batteryData.alarms.chargeCurrent">{{ $t('battery.ok') }}</template>
                                <template v-else>{{ $t('battery.alarm') }}</template>
                            </span>
                          </td>
                          <td>
                              <span class="badge" :class="{
                                  'text-bg-warning text-dark': batteryData.warnings.chargeCurrent,
                                  'text-bg-success': !batteryData.warnings.chargeCurrent
                              }">
                                  <template v-if="!batteryData.warnings.chargeCurrent">{{ $t('battery.ok') }}</template>
                                  <template v-else>{{ $t('battery.warning') }}</template>
                              </span>
                          </td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.lowTemperature') }}</th>
                          <td>
                            <span class="badge" :class="{
                                'text-bg-danger': batteryData.alarms.lowTemperature,
                                'text-bg-success': !batteryData.alarms.lowTemperature
                            }">
                                <template v-if="!batteryData.alarms.lowTemperature">{{ $t('battery.ok') }}</template>
                                <template v-else>{{ $t('battery.alarm') }}</template>
                            </span>
                          </td>
                          <td>
                              <span class="badge" :class="{
                                  'text-bg-warning text-dark': batteryData.warnings.lowTemperature,
                                  'text-bg-success': !batteryData.warnings.lowTemperature
                              }">
                                  <template v-if="!batteryData.warnings.lowTemperature">{{ $t('battery.ok') }}</template>
                                  <template v-else>{{ $t('battery.warning') }}</template>
                              </span>
                          </td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.highTemperature') }}</th>
                          <td>
                            <span class="badge" :class="{
                                'text-bg-danger': batteryData.alarms.highTemperature,
                                'text-bg-success': !batteryData.alarms.highTemperature
                            }">
                                <template v-if="!batteryData.alarms.highTemperature">{{ $t('battery.ok') }}</template>
                                <template v-else>{{ $t('battery.alarm') }}</template>
                            </span>
                          </td>
                          <td>
                              <span class="badge" :class="{
                                  'text-bg-warning text-dark': batteryData.warnings.highTemperature,
                                  'text-bg-success': !batteryData.warnings.highTemperature
                              }">
                                  <template v-if="!batteryData.warnings.highTemperature">{{ $t('battery.ok') }}</template>
                                  <template v-else>{{ $t('battery.warning') }}</template>
                              </span>
                          </td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.lowVoltage') }}</th>
                          <td>
                            <span class="badge" :class="{
                                'text-bg-danger': batteryData.alarms.lowVoltage,
                                'text-bg-success': !batteryData.alarms.lowVoltage
                            }">
                                <template v-if="!batteryData.alarms.lowVoltage">{{ $t('battery.ok') }}</template>
                                <template v-else>{{ $t('battery.alarm') }}</template>
                            </span>
                          </td>
                          <td>
                              <span class="badge" :class="{
                                  'text-bg-warning text-dark': batteryData.warnings.lowVoltage,
                                  'text-bg-success': !batteryData.warnings.lowVoltage
                              }">
                                  <template v-if="!batteryData.warnings.lowVoltage">{{ $t('battery.ok') }}</template>
                                  <template v-else>{{ $t('battery.warning') }}</template>
                              </span>
                          </td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.highVoltage') }}</th>
                          <td>
                            <span class="badge" :class="{
                                'text-bg-danger': batteryData.alarms.highVoltage,
                                'text-bg-success': !batteryData.alarms.highVoltage
                            }">
                                <template v-if="!batteryData.alarms.highVoltage">{{ $t('battery.ok') }}</template>
                                <template v-else>{{ $t('battery.alarm') }}</template>
                            </span>
                          </td>
                          <td>
                              <span class="badge" :class="{
                                  'text-bg-warning text-dark': batteryData.warnings.highVoltage,
                                  'text-bg-success': !batteryData.warnings.highVoltage
                              }">
                                  <template v-if="!batteryData.warnings.highVoltage">{{ $t('battery.ok') }}</template>
                                  <template v-else>{{ $t('battery.warning') }}</template>
                              </span>
                          </td>
                        </tr>
                        <tr>
                          <th scope="row">{{ $t('battery.bmsInternal') }}</th>
                          <td>
                            <span class="badge" :class="{
                                'text-bg-danger': batteryData.alarms.bmsInternal,
                                'text-bg-success': !batteryData.alarms.bmsInternal
                            }">
                                <template v-if="!batteryData.alarms.bmsInternal">{{ $t('battery.ok') }}</template>
                                <template v-else>{{ $t('battery.alarm') }}</template>
                            </span>
                          </td>
                          <td>
                              <span class="badge" :class="{
                                  'text-bg-warning text-dark': batteryData.warnings.bmsInternal,
                                  'text-bg-success': !batteryData.warnings.bmsInternal
                              }">
                                  <template v-if="!batteryData.warnings.bmsInternal">{{ $t('battery.ok') }}</template>
                                  <template v-else>{{ $t('battery.warning') }}</template>
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

  </template>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import type { Battery } from '@/types/BatteryDataStatus';
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
      batteryData: {} as Battery,
      isFirstFetchAfterConnect: true,

      alertMessageLimit: "",
      alertTypeLimit: "info",
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
      console.log("Get initalData for Battery");
      this.dataLoading = true;

      fetch("/api/battery/livedata", { headers: authHeader() })
        .then((response) => handleResponse(response, this.$emitter, this.$router))
        .then((data) => {
          this.batteryData = data;
          this.dataLoading = false;
        });
    },
    initSocket() {
      console.log("Starting connection to Battery WebSocket Server");

      const { protocol, host } = location;
      const authString = authUrl();
      const webSocketUrl = `${protocol === "https:" ? "wss" : "ws"
        }://${authString}${host}/batterylivedata`;

      this.socket = new WebSocket(webSocketUrl);

      this.socket.onmessage = (event) => {
        console.log(event);
        this.batteryData = JSON.parse(event.data);
        this.dataLoading = false;
        this.heartCheck(); // Reset heartbeat detection
      };

      this.socket.onopen = function (event) {
        console.log(event);
        console.log("Successfully connected to the Battery websocket server...");
      };

      // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
      window.onbeforeunload = () => {
        this.closeSocket();
      };
    },
    initDataAgeing() {
      this.dataAgeInterval = setInterval(()  => {
        if (this.batteryData) {
          this.batteryData.data_age++;
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
    }
  },
});
</script>