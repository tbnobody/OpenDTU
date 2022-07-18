<template>
    <div class="container-xxl" role="main">
        <div class="page-header">
            <h1>MqTT Info</h1>
        </div>

        <div class="text-center" v-if="dataLoading">
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </div>

        <template v-if="!dataLoading">
            <div class="card">
                <div class="card-header text-white bg-primary">Configuration Summary</div>
                <div class="card-body">
                    <div class="table-responsive">
                        <table class="table table-hover table-condensed">
                            <tbody>
                                <tr>
                                    <th>Status</th>
                                    <td class="badge" :class="{
                                        'bg-danger': !mqttDataList.mqtt_enabled,
                                        'bg-success': mqttDataList.mqtt_enabled,
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
                                        'bg-danger': !mqttDataList.mqtt_retain,
                                        'bg-success': mqttDataList.mqtt_retain,
                                    }">
                                        <span v-if="mqttDataList.mqtt_retain">enabled</span>
                                        <span v-else>disabled</span>
                                    </td>
                                </tr>
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>

            <div class="card mt-5">
                <div class="card-header text-white bg-primary">Home Assistant MQTT Auto Discovery Configuration Summary</div>
                <div class="card-body">
                    <div class="table-responsive">
                        <table class="table table-hover table-condensed">
                            <tbody>
                                <tr>
                                    <th>Status</th>
                                    <td class="badge" :class="{
                                        'bg-danger': !mqttDataList.mqtt_hass_enabled,
                                        'bg-success': mqttDataList.mqtt_hass_enabled,
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
                                        'bg-danger': !mqttDataList.mqtt_hass_retain,
                                        'bg-success': mqttDataList.mqtt_hass_retain,
                                    }">
                                        <span v-if="mqttDataList.mqtt_hass_retain">enabled</span>
                                        <span v-else>disabled</span>
                                    </td>
                                </tr>
                                <tr>
                                    <th>Individual Panels</th>
                                    <td class="badge" :class="{
                                        'bg-danger': !mqttDataList.mqtt_hass_individualpanels,
                                        'bg-success': mqttDataList.mqtt_hass_individualpanels,
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
                <div class="card-header text-white bg-primary">Runtime Summary</div>
                <div class="card-body">
                    <div class="table-responsive">
                        <table class="table table-hover table-condensed">
                            <tbody>
                                <tr>
                                    <th>Connection Status</th>
                                    <td class="badge" :class="{
                                        'bg-danger': !mqttDataList.mqtt_connected,
                                        'bg-success': mqttDataList.mqtt_connected,
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
        </template>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    data() {
        return {
            dataLoading: true,
            mqttDataList: {
                mqtt_enabled: false,
                mqtt_hostname: "",
                mqtt_port: 0,
                mqtt_username: "",
                mqtt_topic: "",
                mqtt_publish_interval: 0,
                mqtt_retain: false,
                mqtt_connected: false,
                mqtt_hass_enabled: false,
                mqtt_hass_retain: false,
                mqtt_hass_topic: "",
                mqtt_hass_individualpanels: false
            },
        };
    },
    created() {
        this.getNtpInfo();
    },
    methods: {
        getNtpInfo() {
            this.dataLoading = true;
            fetch("/api/mqtt/status")
                .then((response) => response.json())
                .then((data) => {
                    this.mqttDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>
