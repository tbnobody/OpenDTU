<template>
    <BasePage
        :title="$t('mqttinfo.MqttInformation')"
        :isLoading="dataLoading"
        :show-reload="true"
        @reload="getMqttInfo"
    >
        <CardElement :text="$t('mqttinfo.ConfigurationSummary')" textVariant="text-bg-primary">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('mqttinfo.Status') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_enabled"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Server') }}</th>
                            <td>{{ mqttDataList.mqtt_hostname }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Port') }}</th>
                            <td>{{ mqttDataList.mqtt_port }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.ClientId') }}</th>
                            <td>{{ mqttDataList.mqtt_clientid }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Username') }}</th>
                            <td>{{ mqttDataList.mqtt_username }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.BaseTopic') }}</th>
                            <td>{{ mqttDataList.mqtt_topic }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.PublishInterval') }}</th>
                            <td>
                                {{
                                    $t('mqttinfo.Seconds', {
                                        sec: mqttDataList.mqtt_publish_interval,
                                    })
                                }}
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.CleanSession') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_clean_session"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Retain') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_retain"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Tls') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_tls"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr v-show="mqttDataList.mqtt_tls">
                            <th>{{ $t('mqttinfo.RootCertifcateInfo') }}</th>
                            <td>{{ mqttDataList.mqtt_root_ca_cert_info }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.TlsCertLogin') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_tls_cert_login"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr v-show="mqttDataList.mqtt_tls_cert_login">
                            <th>{{ $t('mqttinfo.ClientCertifcateInfo') }}</th>
                            <td>{{ mqttDataList.mqtt_client_cert_info }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>

        <CardElement :text="$t('mqttinfo.HassSummary')" textVariant="text-bg-primary" add-space>
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('mqttinfo.Status') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_hass_enabled"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.BaseTopic') }}</th>
                            <td>{{ mqttDataList.mqtt_hass_topic }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Retain') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_hass_retain"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Expire') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_hass_expire"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.IndividualPanels') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_hass_individualpanels"
                                    true_text="mqttinfo.Enabled"
                                    false_text="mqttinfo.Disabled"
                                />
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>

        <CardElement :text="$t('mqttinfo.RuntimeSummary')" textVariant="text-bg-primary" add-space>
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('mqttinfo.ConnectionStatus') }}</th>
                            <td>
                                <StatusBadge
                                    :status="mqttDataList.mqtt_connected"
                                    true_text="mqttinfo.Connected"
                                    false_text="mqttinfo.Disconnected"
                                />
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import CardElement from '@/components/CardElement.vue';
import StatusBadge from '@/components/StatusBadge.vue';
import type { MqttStatus } from '@/types/MqttStatus';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        CardElement,
        StatusBadge,
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
            fetch('/api/mqtt/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.mqttDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>
