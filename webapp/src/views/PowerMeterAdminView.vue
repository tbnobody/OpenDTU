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
                            <li>http://admin:secret@shelly3em.home/status</li>
                            <li>https://admin:secret@shelly3em.home/status</li>
                            <li>http://tasmota-123.home/cm?cmnd=status%208</li>
                            <li>http://12.34.56.78/emeter/0</li>
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
                            v-for="(http_phase, index) in powerMeterConfigList.http_phases"
                            :key="http_phase.index"
                            :text="$t('powermeteradmin.httpPhase', { phaseNumber: http_phase.index })"
                            textVariant="text-bg-primary"
                            add-space>
                        <InputElement
                            v-if="index > 0"
                            :label="$t('powermeteradmin.httpEnabled')"
                            v-model="http_phase.enabled"
                            type="checkbox" wide />

                        <div v-if="http_phase.enabled">
                            <div v-if="index == 0 || powerMeterConfigList.http_individual_requests">
                                <InputElement :label="$t('powermeteradmin.httpUrl')"
                                    v-model="http_phase.url"
                                    type="text"
                                    maxlength="1024"
                                    placeholder="http://admin:supersecret@mypowermeter.home/status"
                                    prefix="GET "
                                    :tooltip="$t('powermeteradmin.httpUrlDescription')" />

                                <div class="row mb-3">
                                    <label for="inputTimezone" class="col-sm-2 col-form-label">{{ $t('powermeteradmin.httpAuthorization') }}</label>
                                    <div class="col-sm-10">
                                        <select class="form-select" v-model="http_phase.auth_type">
                                            <option v-for="source in powerMeterAuthList" :key="source.key" :value="source.key">
                                                {{ source.value }}
                                            </option>
                                        </select>
                                    </div>
                                </div>
                                <div v-if="http_phase.auth_type != 0">
                                    <InputElement :label="$t('powermeteradmin.httpUsername')"
                                        v-model="http_phase.username"
                                        type="text" maxlength="64"/>

                                    <InputElement :label="$t('powermeteradmin.httpPassword')"
                                        v-model="http_phase.password"
                                        type="password" maxlength="64"/>
                                </div>

                                <InputElement :label="$t('powermeteradmin.httpHeaderKey')"
                                    v-model="http_phase.header_key"
                                    type="text"
                                    maxlength="64"
                                    :tooltip="$t('powermeteradmin.httpHeaderKeyDescription')" />

                                <InputElement :label="$t('powermeteradmin.httpHeaderValue')"
                                    v-model="http_phase.header_value"
                                    type="text"
                                    maxlength="256" />

                                <InputElement :label="$t('powermeteradmin.httpTimeout')"
                                    v-model="http_phase.timeout"
                                    type="number"
                                    :postfix="$t('powermeteradmin.milliSeconds')" />
                            </div>

                            <InputElement :label="$t('powermeteradmin.httpJsonPath')"
                                v-model="http_phase.json_path"
                                type="text"
                                maxlength="256"
                                placeholder="total_power"
                                :tooltip="$t('powermeteradmin.httpJsonPathDescription')" />

                            <div class="row mb-3">
                                <label for="power_unit" class="col-sm-2 col-form-label">
                                    {{ $t('powermeteradmin.httpUnit') }}
                                </label>
                                <div class="col-sm-10">
                                    <select id="power_unit" class="form-select" v-model="http_phase.unit">
                                        <option value="1">mW</option>
                                        <option value="0">W</option>
                                        <option value="2">kW</option>
                                    </select>
                                </div>
                            </div>

                            <InputElement
                                :label="$t('powermeteradmin.httpSignInverted')"
                                v-model="http_phase.sign_inverted"
                                :tooltip="$t('powermeteradmin.httpSignInvertedHint')"
                                type="checkbox" />

                            <div class="text-center mb-3">
                                <button type="button" class="btn btn-danger" @click="testHttpRequest(index)">
                                    {{ $t('powermeteradmin.testHttpRequest') }}
                                </button>
                            </div>

                            <BootstrapAlert v-model="testHttpRequestAlert[index].show" dismissible :variant="testHttpRequestAlert[index].type">
                                {{ testHttpRequestAlert[index].message }}
                            </BootstrapAlert>
                        </div>
                    </CardElement>
                </div>

                <div v-if="powerMeterConfigList.source === 6">
                    <CardElement :text="$t('powermeteradmin.TIBBER')"
                            textVariant="text-bg-primary"
                            add-space>

                        <InputElement :label="$t('powermeteradmin.httpUrl')"
                            v-model="powerMeterConfigList.tibber.url"
                            type="text"
                            maxlength="1024"
                            placeholder="http://admin:supersecret@mypowermeter.home/status"
                            prefix="GET "
                            :tooltip="$t('powermeteradmin.httpUrlDescription')" />

                        <InputElement :label="$t('powermeteradmin.httpUsername')"
                            v-model="powerMeterConfigList.tibber.username"
                            type="text" maxlength="64"/>

                        <InputElement :label="$t('powermeteradmin.httpPassword')"
                            v-model="powerMeterConfigList.tibber.password"
                            type="password" maxlength="64"/>

                        <InputElement :label="$t('powermeteradmin.httpTimeout')"
                            v-model="powerMeterConfigList.tibber.timeout"
                            type="number"
                            :postfix="$t('powermeteradmin.milliSeconds')" />

                        <div class="text-center mb-3">
                            <button type="button" class="btn btn-danger" @click="testTibberRequest()">
                                {{ $t('powermeteradmin.testHttpRequest') }}
                            </button>
                        </div>

                        <BootstrapAlert v-model="testTibberRequestAlert.show" dismissible :variant="testTibberRequestAlert.type">
                            {{ testTibberRequestAlert.message }}
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
import { handleResponse, authHeader } from '@/utils/authentication';
import type { PowerMeterHttpPhaseConfig, PowerMeterConfig } from "@/types/PowerMeterConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
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
                { key: 3, value: this.$t('powermeteradmin.typeHTTP') },
                { key: 4, value: this.$t('powermeteradmin.typeSML') },
                { key: 5, value: this.$t('powermeteradmin.typeSMAHM2') },
                { key: 6, value: this.$t('powermeteradmin.typeTIBBER') },
            ],
            powerMeterAuthList: [
                { key: 0, value: "None" },
                { key: 1, value: "Basic" },
                { key: 2, value: "Digest" },
            ],
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            testHttpRequestAlert:  [{message: "", type: "", show: false}] as { message: string; type: string; show: boolean; }[],
            testTibberRequestAlert:  {message: "", type: "", show: false} as { message: string; type: string; show: boolean; }
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

                    for (let i = 0; i < this.powerMeterConfigList.http_phases.length; i++) {
                        this.testHttpRequestAlert.push({
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
        testHttpRequest(index: number) {
            let phaseConfig:PowerMeterHttpPhaseConfig;

            if (this.powerMeterConfigList.http_individual_requests) {
                phaseConfig = this.powerMeterConfigList.http_phases[index];
            } else {
                phaseConfig = { ...this.powerMeterConfigList.http_phases[0] };
                phaseConfig.index = this.powerMeterConfigList.http_phases[index].index;
                phaseConfig.json_path = this.powerMeterConfigList.http_phases[index].json_path;
            }

            this.testHttpRequestAlert[index] = {
                message: "Sending HTTP request...",
                type: "info",
                show: true,
            };

            const formData = new FormData();
            formData.append("data", JSON.stringify(phaseConfig));

            fetch("/api/powermeter/testhttprequest", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.testHttpRequestAlert[index] = {
                            message: response.message,
                            type: response.type,
                            show: true,
                        };
                    }
                )
        },
        testTibberRequest() {
            this.testTibberRequestAlert = {
                message: "Sending Tibber request...",
                type: "info",
                show: true,
            };

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.powerMeterConfigList.tibber));

            fetch("/api/powermeter/testtibberrequest", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.testTibberRequestAlert = {
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
