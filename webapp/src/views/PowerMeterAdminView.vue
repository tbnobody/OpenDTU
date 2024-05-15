<template>
    <BasePage :title="$t('powermeteradmin.PowerMeterSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType" ref="alert">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="savePowerMeterConfig">
            <CardElement :text="$t('powermeteradmin.PowerMeterConfiguration')"
                    textVariant="text-bg-primary">
                <div class="row mb-3">
                    <label class="col-sm-2 form-check-label" for="inputPowerMeterEnable">{{ $t('powermeteradmin.PowerMeterEnable') }}</label>
                    <div class="col-sm-10">
                        <div class="form-check form-switch">
                            <input class="form-check-input" type="checkbox" id="inputPowerMeterEnable"
                                v-model="powerMeterConfigList.enabled" />
                        </div>
                    </div>
                </div>

                <InputElement v-show="powerMeterConfigList.enabled"
                              :label="$t('powermeteradmin.VerboseLogging')"
                              v-model="powerMeterConfigList.verbose_logging"
                              type="checkbox"/>

                <div class="row mb-3" v-show="powerMeterConfigList.enabled">
                    <label for="inputTimezone" class="col-sm-2 col-form-label">{{ $t('powermeteradmin.PowerMeterSource') }}</label>
                    <div class="col-sm-10">
                        <select class="form-select" v-model="powerMeterConfigList.source">
                            <option v-for="source in powerMeterSourceList" :key="source.key" :value="source.key">
                                {{ source.value }}
                            </option>
                        </select>
                    </div>
                </div>
            </CardElement>

            <div v-if="powerMeterConfigList.enabled">
                <CardElement v-if="powerMeterConfigList.source === 0"
                        :text="$t('powermeteradmin.MQTT')"
                        textVariant="text-bg-primary"
                        add-space>
                    <div class="row mb-3">
                        <label for="inputMqttTopicPowerMeter1" class="col-sm-2 col-form-label">{{ $t('powermeteradmin.MqttTopicPowerMeter1') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="text" class="form-control" id="inputMqttTopicPowerMeter1"
                                    placeholder="shellies/shellyem3/emeter/0/power" v-model="powerMeterConfigList.mqtt_topic_powermeter_1" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputMqttTopicPowerMeter2" class="col-sm-2 col-form-label">{{ $t('powermeteradmin.MqttTopicPowerMeter2') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="text" class="form-control" id="inputMqttTopicPowerMeter2"
                                    placeholder="shellies/shellyem3/emeter/1/power" v-model="powerMeterConfigList.mqtt_topic_powermeter_2" />
                            </div>
                        </div>
                    </div>

                    <div class="row mb-3">
                        <label for="inputMqttTopicPowerMeter3" class="col-sm-2 col-form-label">{{ $t('powermeteradmin.MqttTopicPowerMeter3') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="text" class="form-control" id="inputMqttTopicPowerMeter3"
                                    placeholder="shellies/shellyem3/emeter/2/power" v-model="powerMeterConfigList.mqtt_topic_powermeter_3" />
                            </div>
                        </div>
                    </div>
                </CardElement>

                <CardElement v-if="(powerMeterConfigList.source === 1 || powerMeterConfigList.source === 2)"
                        :text="$t('powermeteradmin.SDM')"
                        textVariant="text-bg-primary"
                        add-space>
                    <div class="row mb-3">
                        <label for="sdmaddress" class="col-sm-2 col-form-label">{{ $t('powermeteradmin.sdmaddress') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="text" class="form-control" id="sdmaddress"
                                    placeholder="1" v-model="powerMeterConfigList.sdmaddress" />
                            </div>
                        </div>
                    </div>
                </CardElement>

                <div v-if="powerMeterConfigList.source === 3">
                    <CardElement :text="$t('powermeteradmin.HTTP')"
                            textVariant="text-bg-primary"
                            add-space>
                        <InputElement :label="$t('powermeteradmin.httpIndividualRequests')"
                            v-model="powerMeterConfigList.http_individual_requests"
                            type="checkbox"
                            wide />
                    </CardElement>

                    <div class="alert alert-secondary mt-5" role="alert">
                        <h2>{{ $t('powermeteradmin.urlExamplesHeading') }}:</h2>
                        <ul>
                            <li>http://shelly3em.home/status</li>
                            <li>https://shelly3em.home/status</li>
                            <li>http://tasmota-123.home/cm?cmnd=status%208</li>
                            <li>http://12.34.56.78:8080/emeter/0</li>
                        </ul>

                        <h2>{{ $t('powermeteradmin.jsonPathExamplesHeading') }}:</h2>
                        {{ $t('powermeteradmin.jsonPathExamplesExplanation') }}
                        <ul>
                            <li><code>power/total/watts</code> &mdash; <code>{ "power": { "phase1": { "factor": 0.98, "watts": 42 }, "total": { "watts": 123.4 } } }</code></li>
                            <li><code>data/[1]/power</code> &mdash; <code>{ "data": [ { "factor": 0.98, "power": 42 }, { "factor": 1.0, "power": 123.4 } ] } }</code></li>
                            <li><code>total</code> &mdash; <code>{ "othervalue": 66, "total": 123.4 }</code></li>
                        </ul>
                    </div>

                    <CardElement
                            v-for="(httpJson, index) in powerMeterConfigList.http_json"
                            :key="httpJson.index"
                            :text="$t('powermeteradmin.httpValue', { valueNumber: httpJson.index })"
                            textVariant="text-bg-primary"
                            add-space>
                        <InputElement
                            v-if="index > 0"
                            :label="$t('powermeteradmin.httpEnabled')"
                            v-model="httpJson.enabled"
                            type="checkbox" wide />

                        <div v-if="httpJson.enabled">

                            <HttpRequestSettings :cfg="httpJson.http_request" v-if="index == 0 || powerMeterConfigList.http_individual_requests"/>

                            <InputElement :label="$t('powermeteradmin.httpJsonPath')"
                                v-model="httpJson.json_path"
                                type="text"
                                maxlength="256"
                                placeholder="total_power"
                                :tooltip="$t('powermeteradmin.httpJsonPathDescription')" />

                            <div class="row mb-3">
                                <label for="power_unit" class="col-sm-2 col-form-label">
                                    {{ $t('powermeteradmin.httpUnit') }}
                                </label>
                                <div class="col-sm-10">
                                    <select id="power_unit" class="form-select" v-model="httpJson.unit">
                                        <option v-for="u in unitTypeList" :key="u.key" :value="u.key">
                                            {{ u.value }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <InputElement
                                :label="$t('powermeteradmin.httpSignInverted')"
                                v-model="httpJson.sign_inverted"
                                :tooltip="$t('powermeteradmin.httpSignInvertedHint')"
                                type="checkbox" />

                            <div class="text-center mb-3">
                                <button type="button" class="btn btn-danger" @click="testHttpJsonRequest(index)">
                                    {{ $t('powermeteradmin.testHttpJsonRequest') }}
                                </button>
                            </div>

                            <BootstrapAlert v-model="testHttpJsonRequestAlert[index].show" dismissible :variant="testHttpJsonRequestAlert[index].type">
                                {{ testHttpJsonRequestAlert[index].message }}
                            </BootstrapAlert>
                        </div>
                    </CardElement>
                </div>

                <div v-if="powerMeterConfigList.source === 6">
                    <CardElement :text="$t('powermeteradmin.HTTP_SML')"
                            textVariant="text-bg-primary"
                            add-space>

                        <HttpRequestSettings :cfg="powerMeterConfigList.http_sml.http_request" />

                        <div class="text-center mb-3">
                            <button type="button" class="btn btn-danger" @click="testHttpSmlRequest()">
                                {{ $t('powermeteradmin.testHttpSmlRequest') }}
                            </button>
                        </div>

                        <BootstrapAlert v-model="testHttpSmlRequestAlert.show" dismissible :variant="testHttpSmlRequestAlert.type">
                            {{ testHttpSmlRequestAlert.message }}
                        </BootstrapAlert>
                    </CardElement>
                </div>
            </div>

            <FormFooter @reload="getPowerMeterConfig"/>

        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import HttpRequestSettings from '@/components/HttpRequestSettings.vue';
import { handleResponse, authHeader } from '@/utils/authentication';
import type { PowerMeterHttpJsonConfig, PowerMeterConfig } from "@/types/PowerMeterConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
        HttpRequestSettings,
        InputElement
    },
    data() {
        return {
            dataLoading: true,
            powerMeterConfigList: {} as PowerMeterConfig,
            powerMeterSourceList: [
                { key: 0, value: this.$t('powermeteradmin.typeMQTT') },
                { key: 1, value: this.$t('powermeteradmin.typeSDM1ph') },
                { key: 2, value: this.$t('powermeteradmin.typeSDM3ph') },
                { key: 3, value: this.$t('powermeteradmin.typeHTTP_JSON') },
                { key: 4, value: this.$t('powermeteradmin.typeSML') },
                { key: 5, value: this.$t('powermeteradmin.typeSMAHM2') },
                { key: 6, value: this.$t('powermeteradmin.typeHTTP_SML') },
            ],
            unitTypeList: [
                { key: 1, value: "mW" },
                { key: 0, value: "W" },
                { key: 2, value: "kW" },
            ],
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            testHttpJsonRequestAlert:  [{message: "", type: "", show: false}] as { message: string; type: string; show: boolean; }[],
            testHttpSmlRequestAlert:  {message: "", type: "", show: false} as { message: string; type: string; show: boolean; }
        };
    },
    created() {
        this.getPowerMeterConfig();
    },
    methods: {
        getPowerMeterConfig() {
            this.dataLoading = true;
            fetch("/api/powermeter/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.powerMeterConfigList = data;
                    this.dataLoading = false;

                    for (let i = 0; i < this.powerMeterConfigList.http_json.length; i++) {
                        this.testHttpJsonRequestAlert.push({
                            message: "",
                            type: "",
                            show: false,
                        });
                    }
                });
        },
        savePowerMeterConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.powerMeterConfigList));

            fetch("/api/powermeter/config", {
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
                        window.scrollTo(0, 0);
                    }
                );
        },
        testHttpJsonRequest(index: number) {
            let valueConfig:PowerMeterHttpJsonConfig;

            if (this.powerMeterConfigList.http_individual_requests) {
                valueConfig = this.powerMeterConfigList.http_json[index];
            } else {
                valueConfig = { ...this.powerMeterConfigList.http_json[0] };
                valueConfig.index = this.powerMeterConfigList.http_json[index].index;
                valueConfig.json_path = this.powerMeterConfigList.http_json[index].json_path;
            }

            this.testHttpJsonRequestAlert[index] = {
                message: "Sending HTTP request...",
                type: "info",
                show: true,
            };

            const formData = new FormData();
            formData.append("data", JSON.stringify(valueConfig));

            fetch("/api/powermeter/testhttpjsonrequest", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.testHttpJsonRequestAlert[index] = {
                            message: response.message,
                            type: response.type,
                            show: true,
                        };
                    }
                )
        },
        testHttpSmlRequest() {
            this.testHttpSmlRequestAlert = {
                message: "Sending HTTP SML request...",
                type: "info",
                show: true,
            };

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.powerMeterConfigList.http_sml));

            fetch("/api/powermeter/testhttpsmlrequest", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.testHttpSmlRequestAlert = {
                            message: response.message,
                            type: response.type,
                            show: true,
                        };
                    }
                )
        },
    },
});
</script>
