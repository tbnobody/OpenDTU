<template>
    <BasePage :title="'MqTT Info'" :isLoading="dataLoading">
        <div class="card">
            <div class="card-header text-bg-primary">Configuration Summary</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Status</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !mqttDataList.mqtt_enabled,
                                    'text-bg-success': mqttDataList.mqtt_enabled,
                                }">
                                    <span v-if="mqttDataList.mqtt_enabled">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                            <tr>
                                <th>Server</th>
                                <td>{{ mqttDataList.mqtt_hostname }}</td>
                            </tr>
                            <tr>
                                <th>Port</th>
                                <td>{{ mqttDataList.mqtt_port }}</td>
                            </tr>
                            <tr>
                                <th>Username</th>
                                <td>{{ mqttDataList.mqtt_username }}</td>
                            </tr>
                            <tr>
                                <th>Base Topic</th>
                                <td>{{ mqttDataList.mqtt_topic }}</td>
                            </tr>
                            <tr>
                                <th>Publish Interval</th>
                                <td>{{ mqttDataList.mqtt_publish_interval }} seconds</td>
                            </tr>
                            <tr>
                                <th>Retain</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !mqttDataList.mqtt_retain,
                                    'text-bg-success': mqttDataList.mqtt_retain,
                                }">
                                    <span v-if="mqttDataList.mqtt_retain">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                            <tr>
                                <th>TLS</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !mqttDataList.mqtt_tls,
                                    'text-bg-success': mqttDataList.mqtt_tls,
                                }">
                                    <span v-if="mqttDataList.mqtt_tls">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                            <tr v-show="mqttDataList.mqtt_tls">
                                <th>Root CA Certifcate Info</th>
                                <td>{{ mqttDataList.mqtt_root_ca_cert_info }}</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>

        <div class="card mt-5">
            <div class="card-header text-bg-primary">Home Assistant MQTT Auto Discovery Configuration Summary
            </div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Status</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !mqttDataList.mqtt_hass_enabled,
                                    'text-bg-success': mqttDataList.mqtt_hass_enabled,
                                }">
                                    <span v-if="mqttDataList.mqtt_hass_enabled">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                            <tr>
                                <th>Base Topic</th>
                                <td>{{ mqttDataList.mqtt_hass_topic }}</td>
                            </tr>
                            <tr>
                                <th>Retain</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !mqttDataList.mqtt_hass_retain,
                                    'text-bg-success': mqttDataList.mqtt_hass_retain,
                                }">
                                    <span v-if="mqttDataList.mqtt_hass_retain">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                            <tr>
                                <th>Expire</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !mqttDataList.mqtt_hass_expire,
                                    'text-bg-success': mqttDataList.mqtt_hass_expire,
                                }">
                                    <span v-if="mqttDataList.mqtt_hass_expire">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                            <tr>
                                <th>Individual Panels</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !mqttDataList.mqtt_hass_individualpanels,
                                    'text-bg-success': mqttDataList.mqtt_hass_individualpanels,
                                }">
                                    <span v-if="mqttDataList.mqtt_hass_individualpanels">enabled</span>
                                    <span v-else>disabled</span>
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>

        <div class="card mt-5">
            <div class="card-header text-bg-primary">Runtime Summary</div>
            <div class="card-body">
                <div class="table-responsive">
                    <table class="table table-hover table-condensed">
                        <tbody>
                            <tr>
                                <th>Connection Status</th>
                                <td class="badge" :class="{
                                    'text-bg-danger': !mqttDataList.mqtt_connected,
                                    'text-bg-success': mqttDataList.mqtt_connected,
                                }">
                                    <span v-if="mqttDataList.mqtt_connected">connected</span>
                                    <span v-else>disconnected</span>
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { handleResponse, authHeader } from '@/utils/authentication';
import BasePage from '@/components/BasePage.vue';
import type { MqttStatus } from '@/types/MqttStatus';

export default defineComponent({
    components: {
        BasePage,
    },
    data() {
        return {
            dataLoading: true,
            mqttDataList: {} as MqttStatus,
        };
    },
    created() {
        this.getMqttInfo();
    },
    methods: {
        getMqttInfo() {
            this.dataLoading = true;
            fetch("/api/mqtt/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.mqttDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>
