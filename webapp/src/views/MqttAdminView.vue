<template>
    <BasePage :title="'MqTT Settings'" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveMqttConfig">
            <div class="card">
                <div class="card-header text-bg-primary">MqTT Configuration</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label class="col-sm-4 form-check-label" for="inputMqtt">Enable MqTT</label>
                        <div class="col-sm-8">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputMqtt"
                                    v-model="mqttConfigList.mqtt_enabled" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3" v-show="mqttConfigList.mqtt_enabled">
                        <label class="col-sm-4 form-check-label" for="inputMqttHass">Enable Home Assistant MQTT Auto
                            Discovery</label>
                        <div class="col-sm-8">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputMqttHass"
                                    v-model="mqttConfigList.mqtt_hass_enabled" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="card mt-5" v-show="mqttConfigList.mqtt_enabled">
                <div class="card-header text-bg-primary">
                    MqTT Broker Parameter
                </div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputHostname" class="col-sm-2 col-form-label">Hostname:</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputHostname" maxlength="128"
                                placeholder="Hostname or IP address" v-model="mqttConfigList.mqtt_hostname" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPort" class="col-sm-2 col-form-label">Port:</label>
                        <div class="col-sm-10">
                            <input type="number" class="form-control" id="inputPort" min="1" max="65535"
                                placeholder="Port number" v-model="mqttConfigList.mqtt_port" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputUsername" class="col-sm-2 col-form-label">Username:</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputUsername" maxlength="64"
                                placeholder="Username, leave empty for anonymous connection"
                                v-model="mqttConfigList.mqtt_username" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPassword" class="col-sm-2 col-form-label">Password:</label>
                        <div class="col-sm-10">
                            <input type="password" class="form-control" id="inputPassword" maxlength="64"
                                placeholder="Password, leave empty for anonymous connection"
                                v-model="mqttConfigList.mqtt_password" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputTopic" class="col-sm-2 col-form-label">Base Topic:</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputTopic" maxlength="32"
                                placeholder="Base topic, will be prepend to all published topics (e.g. inverter/)"
                                v-model="mqttConfigList.mqtt_topic" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPublishInterval" class="col-sm-2 col-form-label">Publish Interval:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="inputPublishInterval" min="5" max="86400"
                                    placeholder="Publish Interval in Seconds"
                                    v-model="mqttConfigList.mqtt_publish_interval"
                                    aria-describedby="publishIntervalDescription" />
                                <span class="input-group-text" id="publishIntervalDescription">seconds</span>
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputRetain">Enable Retain Flag</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputRetain"
                                    v-model="mqttConfigList.mqtt_retain" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputTls">Enable TLS</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputTls"
                                    v-model="mqttConfigList.mqtt_tls" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3" v-show="mqttConfigList.mqtt_tls">
                        <label for="inputCert" class="col-sm-2 col-form-label">CA-Root-Certificate (default
                            Letsencrypt):</label>
                        <div class="col-sm-10">
                            <textarea class="form-control" id="inputCert" maxlength="2048" rows="10"
                                placeholder="Root CA Certificate from Letsencrypt"
                                v-model="mqttConfigList.mqtt_root_ca_cert">
                                </textarea>
                        </div>
                    </div>

                </div>
            </div>

            <div class="card mt-5" v-show="mqttConfigList.mqtt_enabled">
                <div class="card-header text-bg-primary">LWT Parameters</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputLwtTopic" class="col-sm-2 col-form-label">LWT Topic:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <span class="input-group-text" id="basic-addon3">{{
                                mqttConfigList.mqtt_topic
                                }}</span>
                                <input type="text" class="form-control" id="inputLwtTopic" maxlength="32"
                                    placeholder="LWT topic, will be append base topic"
                                    v-model="mqttConfigList.mqtt_lwt_topic" aria-describedby="basic-addon3" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputLwtOnline" class="col-sm-2 col-form-label">LWT Online message:</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputLwtOnline" maxlength="20"
                                placeholder="Message that will be published to LWT topic when online"
                                v-model="mqttConfigList.mqtt_lwt_online" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputLwtOffline" class="col-sm-2 col-form-label">LWT Offline message:</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputLwtOffline" maxlength="20"
                                placeholder="Message that will be published to LWT topic when offline"
                                v-model="mqttConfigList.mqtt_lwt_offline" />
                        </div>
                    </div>
                </div>
            </div>

            <div class="card mt-5" v-show="mqttConfigList.mqtt_enabled && mqttConfigList.mqtt_hass_enabled">
                <div class="card-header text-bg-primary">Home Assistant MQTT Auto Discovery Parameters</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputHassTopic" class="col-sm-2 col-form-label">Prefix Topic:</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputHassTopic" maxlength="32"
                                placeholder="The prefix for the discovery topic"
                                v-model="mqttConfigList.mqtt_hass_topic" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputHassRetain">Enable Retain Flag</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputHassRetain"
                                    v-model="mqttConfigList.mqtt_hass_retain" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputHassExpire">Enable Expiration</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputHassExpire"
                                    v-model="mqttConfigList.mqtt_hass_expire" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputIndividualPanels">Individual
                            Panels:</label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputIndividualPanels"
                                    v-model="mqttConfigList.mqtt_hass_individualpanels" />
                            </div>
                        </div>
                    </div>

                </div>
            </div>

            <button type="submit" class="btn btn-primary mb-3">Save</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import type { MqttConfig } from "@/types/MqttConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: true,
            mqttConfigList: {} as MqttConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getMqttConfig();
    },
    methods: {
        getMqttConfig() {
            this.dataLoading = true;
            fetch("/api/mqtt/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.mqttConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveMqttConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.mqttConfigList));

            fetch("/api/mqtt/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.alertMessage = response.message;
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>
