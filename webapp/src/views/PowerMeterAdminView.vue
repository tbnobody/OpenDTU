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
                        <label for="sdmbaudrate" class="col-sm-2 col-form-label">{{ $t('powermeteradmin.sdmbaudrate') }}:</label>
                        <div class="col-sm-10">
                            <div class="input-group">
                                <input type="text" class="form-control" id="sdmbaudrate"
                                    placeholder="9600" v-model="powerMeterConfigList.sdmbaudrate" />
                            </div>
                        </div>
                    </div>
                
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
            </div>

            <button type="submit" class="btn btn-primary mb-3">{{ $t('powermeteradmin.Save') }}</button>

            <div v-if="powerMeterConfigList.source === 3" class="alert alert-secondary" role="alert">
                <h2>URL examples:</h2>
                <ul>
                    <li>http://admin:secret@shelly3em.home/status</li>
                    <li>https://admin:secret@shelly3em.home/status</li>
                    <li>http://tasmota-123.home/cm?cmnd=status%208</li>
                    <li>http://12.34.56.78/emeter/0</li>
                </ul>

                <h2>JSON path examples:</h2>
                <ul>
                    <li>total_power - { "othervalue": "blah", "total_power": 123.4 }</li>
                    <li>testarray/[2]/myvalue - { "testarray": [ {}, { "power": 123.4 } ] }</li>
                </ul>

                More info: <a href="https://github.com/mobizt/FirebaseJson">https://github.com/mobizt/FirebaseJson</a>
            </div>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import { handleResponse, authHeader } from '@/utils/authentication';
// import { BIconInfoCircle } from 'bootstrap-icons-vue';
import type { PowerMeterHttpPhaseConfig, PowerMeterConfig } from "@/types/PowerMeterConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        InputElement,
        // BIconInfoCircle,
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
            ],
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            testHttpRequestAlert:  [{message: "", type: "", show: false}] as { message: string; type: string; show: boolean; }[]
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

                    // type MyType = {
                    //     id: number;
                    //     name: string;
                    // }

                    // type MyGroupType = {
                    //     [key:string]: MyType;
                    // }

                    for (var i = 0; i < this.powerMeterConfigList.http_phases.length; i++) {
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
            var phaseConfig:PowerMeterHttpPhaseConfig;

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
    },
});
</script>
