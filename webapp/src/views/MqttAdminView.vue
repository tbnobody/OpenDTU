<template>
    <BasePage :title="$t('mqttadmin.MqttSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveMqttConfig">
            <div class="card">
                <div class="card-header text-bg-primary">{{ $t('mqttadmin.MqttConfiguration') }}</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label class="col-sm-4 form-check-label" for="inputMqtt">
                            {{ $t('mqttadmin.EnableMqtt') }}
                        </label>
                        <div class="col-sm-8">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputMqtt"
                                    v-model="mqttConfigList.mqtt_enabled" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3" v-show="mqttConfigList.mqtt_enabled">
                        <label class="col-sm-4 form-check-label" for="inputMqttHass">
                            {{ $t('mqttadmin.EnableHass') }}
                        </label>
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
                    {{ $t('mqttadmin.MqttBrokerParameter') }}
                </div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputHostname" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.Hostname') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputHostname" maxlength="128"
                                :placeholder="$t('mqttadmin.HostnameHint')" v-model="mqttConfigList.mqtt_hostname" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPort" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.Port') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="number" class="form-control" id="inputPort" min="1" max="65535"
                                v-model="mqttConfigList.mqtt_port" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputUsername" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.Username') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputUsername" maxlength="64"
                                :placeholder="$t('mqttadmin.UsernameHint')" v-model="mqttConfigList.mqtt_username" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPassword" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.Password') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="password" class="form-control" id="inputPassword" maxlength="64"
                                :placeholder="$t('mqttadmin.PasswordHint')" v-model="mqttConfigList.mqtt_password" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputTopic" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.BaseTopic') }}</label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputTopic" maxlength="32"
                                :placeholder="$t('mqttadmin.BaseTopicHint')" v-model="mqttConfigList.mqtt_topic" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputPublishInterval" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.PublishInterval') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="number" class="form-control" id="inputPublishInterval" min="5" max="86400"
                                    placeholder="Publish Interval in Seconds"
                                    v-model="mqttConfigList.mqtt_publish_interval"
                                    aria-describedby="publishIntervalDescription" />
                                <span class="input-group-text" id="publishIntervalDescription">
                                    {{ $t('mqttadmin.Seconds') }}</span>
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputRetain">
                            {{ $t('mqttadmin.EnableRetain') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputRetain"
                                    v-model="mqttConfigList.mqtt_retain" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputTls">
                            {{ $t('mqttadmin.EnableTls') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputTls"
                                    v-model="mqttConfigList.mqtt_tls" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3" v-show="mqttConfigList.mqtt_tls">
                        <label for="inputCert" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.RootCa') }}
                        </label>
                        <div class="col-sm-10">
                            <textarea class="form-control" id="inputCert" maxlength="2048" rows="10"
                                v-model="mqttConfigList.mqtt_root_ca_cert">
                            </textarea>
                        </div>
                    </div>

                </div>
            </div>

            <div class="card mt-5" v-show="mqttConfigList.mqtt_enabled">
                <div class="card-header text-bg-primary">{{ $t('mqttadmin.LwtParameters') }}</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputLwtTopic" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.LwtTopic') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <span class="input-group-text" id="basic-addon3">
                                    {{ mqttConfigList.mqtt_topic }}
                                </span>
                                <input type="text" class="form-control" id="inputLwtTopic" maxlength="32"
                                    :placeholder="$t('mqttadmin.LwtTopicHint')" aria-describedby="basic-addon3"
                                    v-model="mqttConfigList.mqtt_lwt_topic" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputLwtOnline" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.LwtOnline') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputLwtOnline" maxlength="20"
                                :placeholder="$t('mqttadmin.LwtOnlineHint')" v-model="mqttConfigList.mqtt_lwt_online" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputLwtOffline" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.LwtOffline') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputLwtOffline" maxlength="20"
                                :placeholder="$t('mqttadmin.LwtOfflineHint')"
                                v-model="mqttConfigList.mqtt_lwt_offline" />
                        </div>
                    </div>
                </div>
            </div>

            <div class="card mt-5" v-show="mqttConfigList.mqtt_enabled && mqttConfigList.mqtt_hass_enabled">
                <div class="card-header text-bg-primary">{{ $t('mqttadmin.HassParameters') }}</div>
                <div class="card-body">
                    <div class="row mb-3">
                        <label for="inputHassTopic" class="col-sm-2 col-form-label">
                            {{ $t('mqttadmin.HassPrefixTopic') }}
                        </label>
                        <div class="col-sm-10">
                            <input type="text" class="form-control" id="inputHassTopic" maxlength="32"
                                :placeholder="$t('mqttadmin.HassPrefixTopicHint')"
                                v-model="mqttConfigList.mqtt_hass_topic" />
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputHassRetain">
                            {{ $t('mqttadmin.HassRetain') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputHassRetain"
                                    v-model="mqttConfigList.mqtt_hass_retain" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputHassExpire">
                            {{ $t('mqttadmin.HassExpire') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputHassExpire"
                                    v-model="mqttConfigList.mqtt_hass_expire" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label class="col-sm-2 form-check-label" for="inputIndividualPanels">
                            {{ $t('mqttadmin.HassIndividual') }}
                        </label>
                        <div class="col-sm-10">
                            <div class="form-check form-switch">
                                <input class="form-check-input" type="checkbox" id="inputIndividualPanels"
                                    v-model="mqttConfigList.mqtt_hass_individualpanels" />
                            </div>
                        </div>
                    </div>

                </div>
            </div>

            <button type="submit" class="btn btn-primary mb-3">{{ $t('mqttadmin.Save') }}</button>
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
                        this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>
