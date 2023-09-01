<template>
    <BasePage :title="$t('mqttadmin.MqttSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveMqttConfig">
            <CardElement :text="$t('mqttadmin.MqttConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('mqttadmin.EnableMqtt')"
                              v-model="mqttConfigList.mqtt_enabled"
                              type="checkbox" wide/>

                <InputElement v-show="mqttConfigList.mqtt_enabled"
                              :label="$t('mqttadmin.EnableHass')"
                              v-model="mqttConfigList.mqtt_hass_enabled"
                              type="checkbox" wide/>
            </CardElement>

            <CardElement :text="$t('mqttadmin.MqttBrokerParameter')" textVariant="text-bg-primary" add-space
                         v-show="mqttConfigList.mqtt_enabled"
            >
                <InputElement :label="$t('mqttadmin.Hostname')"
                              v-model="mqttConfigList.mqtt_hostname"
                              type="text" maxlength="128"
                              :placeholder="$t('mqttadmin.HostnameHint')"/>

                <InputElement :label="$t('mqttadmin.Port')"
                              v-model="mqttConfigList.mqtt_port"
                              type="number" min="1" max="65535"/>

                <InputElement :label="$t('mqttadmin.Username')"
                              v-model="mqttConfigList.mqtt_username"
                              type="text" maxlength="64"
                              :placeholder="$t('mqttadmin.UsernameHint')"/>

                <InputElement :label="$t('mqttadmin.Password')"
                              v-model="mqttConfigList.mqtt_password"
                              type="password" maxlength="64"
                              :placeholder="$t('mqttadmin.PasswordHint')"/>

                <InputElement :label="$t('mqttadmin.BaseTopic')"
                              v-model="mqttConfigList.mqtt_topic"
                              type="text" maxlength="32"
                              :placeholder="$t('mqttadmin.BaseTopicHint')"/>

                <InputElement :label="$t('mqttadmin.PublishInterval')"
                              v-model="mqttConfigList.mqtt_publish_interval"
                              type="number" min="5" max="86400"
                              :postfix="$t('mqttadmin.Seconds')"/>

                <InputElement :label="$t('mqttadmin.CleanSession')"
                              v-model="mqttConfigList.mqtt_clean_session"
                              type="checkbox"/>

                <InputElement :label="$t('mqttadmin.EnableRetain')"
                              v-model="mqttConfigList.mqtt_retain"
                              type="checkbox"/>

                <InputElement :label="$t('mqttadmin.EnableTls')"
                              v-model="mqttConfigList.mqtt_tls"
                              type="checkbox"/>

                <InputElement v-show="mqttConfigList.mqtt_tls"
                              :label="$t('mqttadmin.RootCa')"
                              v-model="mqttConfigList.mqtt_root_ca_cert"
                              type="textarea" maxlength="2560" rows="10"/>

                <InputElement v-show="mqttConfigList.mqtt_tls"
                              :label="$t('mqttadmin.TlsCertLoginEnable')"
                              v-model="mqttConfigList.mqtt_tls_cert_login"
                              type="checkbox"/>

                <InputElement v-show="mqttConfigList.mqtt_tls_cert_login"
                              :label="$t('mqttadmin.ClientCert')"
                              v-model="mqttConfigList.mqtt_client_cert"
                              type="textarea" maxlength="2560" rows="10"/>

                <InputElement v-show="mqttConfigList.mqtt_tls_cert_login"
                              :label="$t('mqttadmin.ClientKey')"
                              v-model="mqttConfigList.mqtt_client_key"
                              type="textarea" maxlength="2560" rows="10"/>
            </CardElement>

            <CardElement :text="$t('mqttadmin.LwtParameters')" textVariant="text-bg-primary" add-space
                         v-show="mqttConfigList.mqtt_enabled"
            >
                <InputElement :label="$t('mqttadmin.LwtTopic')"
                              v-model="mqttConfigList.mqtt_lwt_topic"
                              type="text" maxlength="32" :prefix="mqttConfigList.mqtt_topic"
                              :placeholder="$t('mqttadmin.LwtTopicHint')"/>

                <InputElement :label="$t('mqttadmin.LwtOnline')"
                              v-model="mqttConfigList.mqtt_lwt_online"
                              type="text" maxlength="20"
                              :placeholder="$t('mqttadmin.LwtOnlineHint')"/>

                <InputElement :label="$t('mqttadmin.LwtOffline')"
                              v-model="mqttConfigList.mqtt_lwt_offline"
                              type="text" maxlength="20"
                              :placeholder="$t('mqttadmin.LwtOfflineHint')"/>
            </CardElement>

            <CardElement :text="$t('mqttadmin.HassParameters')" textVariant="text-bg-primary" add-space
                         v-show="mqttConfigList.mqtt_enabled && mqttConfigList.mqtt_hass_enabled"
            >
                <InputElement :label="$t('mqttadmin.HassPrefixTopic')"
                              v-model="mqttConfigList.mqtt_hass_topic"
                              type="text" maxlength="32"
                              :placeholder="$t('mqttadmin.HassPrefixTopicHint')"/>

                <InputElement :label="$t('mqttadmin.HassRetain')"
                              v-model="mqttConfigList.mqtt_hass_retain"
                              type="checkbox"/>

                <InputElement :label="$t('mqttadmin.HassExpire')"
                              v-model="mqttConfigList.mqtt_hass_expire"
                              type="checkbox"/>

                <InputElement :label="$t('mqttadmin.HassIndividual')"
                              v-model="mqttConfigList.mqtt_hass_individualpanels"
                              type="checkbox"/>
            </CardElement>

            <button type="submit" class="btn btn-primary mb-3">{{ $t('mqttadmin.Save') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import type { MqttConfig } from "@/types/MqttConfig";
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        InputElement,
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
