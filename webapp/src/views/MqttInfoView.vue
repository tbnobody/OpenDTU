<template>
    <BasePage :title="$t('mqttinfo.MqttInformation')" :isLoading="dataLoading">
        <CardElement :text="$t('mqttinfo.ConfigurationSummary')" textVariant="text-bg-primary">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('mqttinfo.Status') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !mqttDataList.mqtt_enabled,
                                'text-bg-success': mqttDataList.mqtt_enabled,
                            }">
                                <span v-if="mqttDataList.mqtt_enabled">{{ $t('mqttinfo.Enabled') }}</span>
                                <span v-else>{{ $t('mqttinfo.Disabled') }}</span>
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
                            <th>{{ $t('mqttinfo.Username') }}</th>
                            <td>{{ mqttDataList.mqtt_username }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.BaseTopic') }}</th>
                            <td>{{ mqttDataList.mqtt_topic }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.PublishInterval') }}</th>
                            <td>{{ $t('mqttinfo.Seconds', { sec: mqttDataList.mqtt_publish_interval }) }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Retain') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !mqttDataList.mqtt_retain,
                                'text-bg-success': mqttDataList.mqtt_retain,
                            }">
                                <span v-if="mqttDataList.mqtt_retain">{{ $t('mqttinfo.Enabled') }}</span>
                                <span v-else>{{ $t('mqttinfo.Disabled') }}</span>
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Tls') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !mqttDataList.mqtt_tls,
                                'text-bg-success': mqttDataList.mqtt_tls,
                            }">
                                <span v-if="mqttDataList.mqtt_tls">{{ $t('mqttinfo.Enabled') }}</span>
                                <span v-else>{{ $t('mqttinfo.Disabled') }}</span>
                            </td>
                        </tr>
                        <tr v-show="mqttDataList.mqtt_tls">
                            <th>{{ $t('mqttinfo.RootCertifcateInfo') }}</th>
                            <td>{{ mqttDataList.mqtt_root_ca_cert_info }}</td>
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
                            <td class="badge" :class="{
                                'text-bg-danger': !mqttDataList.mqtt_hass_enabled,
                                'text-bg-success': mqttDataList.mqtt_hass_enabled,
                            }">
                                <span v-if="mqttDataList.mqtt_hass_enabled">{{ $t('mqttinfo.Enabled') }}</span>
                                <span v-else>{{ $t('mqttinfo.Disabled') }}</span>
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.BaseTopic') }}</th>
                            <td>{{ mqttDataList.mqtt_hass_topic }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Retain') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !mqttDataList.mqtt_hass_retain,
                                'text-bg-success': mqttDataList.mqtt_hass_retain,
                            }">
                                <span v-if="mqttDataList.mqtt_hass_retain">{{ $t('mqttinfo.Enabled') }}</span>
                                <span v-else>{{ $t('mqttinfo.Disabled') }}</span>
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.Expire') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !mqttDataList.mqtt_hass_expire,
                                'text-bg-success': mqttDataList.mqtt_hass_expire,
                            }">
                                <span v-if="mqttDataList.mqtt_hass_expire">{{ $t('mqttinfo.Enabled') }}</span>
                                <span v-else>{{ $t('mqttinfo.Disabled') }}</span>
                            </td>
                        </tr>
                        <tr>
                            <th>{{ $t('mqttinfo.IndividualPanels') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !mqttDataList.mqtt_hass_individualpanels,
                                'text-bg-success': mqttDataList.mqtt_hass_individualpanels,
                            }">
                                <span v-if="mqttDataList.mqtt_hass_individualpanels">{{ $t('mqttinfo.Enabled')
                                }}</span>
                                <span v-else>{{ $t('mqttinfo.Disabled') }}</span>
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
                            <td class="badge" :class="{
                                'text-bg-danger': !mqttDataList.mqtt_connected,
                                'text-bg-success': mqttDataList.mqtt_connected,
                            }">
                                <span v-if="mqttDataList.mqtt_connected">{{ $t('mqttinfo.Connected') }}</span>
                                <span v-else>{{ $t('mqttinfo.Disconnected') }}</span>
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
import type { MqttStatus } from '@/types/MqttStatus';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        CardElement,
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
