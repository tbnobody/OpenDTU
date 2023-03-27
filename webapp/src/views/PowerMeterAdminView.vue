<template>
    <BasePage :title="$t('powermeteradmin.PowerMeterSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>
        
        <form @submit="savePowerMeterConfig">
            <div class="card">
                <div class="card-header text-bg-primary">{{ $t('powermeteradmin.PowerMeterConfiguration') }}</div>
                <div class="card-body">

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
                </div>
            </div>

            <div class="card" v-if="powerMeterConfigList.source === 0 && powerMeterConfigList.enabled" >
                <div class="card-header text-bg-primary">{{ $t('powermeteradmin.MQTT') }}</div>
                <div class="card-body">
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
                </div>
            </div>

            
            <div class="card" v-if="(powerMeterConfigList.source === 1 || powerMeterConfigList.source === 2)  && powerMeterConfigList.enabled" >
                <div class="card-header text-bg-primary">{{ $t('powermeteradmin.SDM') }}</div>
                <div class="card-body">
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
                </div>
            </div>
            <button type="submit" class="btn btn-primary mb-3">{{ $t('powermeteradmin.Save') }}</button>
        </form>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import { handleResponse, authHeader } from '@/utils/authentication';
import type { PowerMeterConfig } from "@/types/PowerMeterConfig";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: true,
            powerMeterConfigList: {} as PowerMeterConfig,
            powerMeterSourceList: [
                { key: 0, value: this.$t('powermeteradmin.typeMQTT') },
                { key: 1, value: this.$t('powermeteradmin.typeSDM1ph') },
                { key: 2, value: this.$t('powermeteradmin.typeSDM3ph') },
            ],
            alertMessage: "",
            alertType: "info",
            showAlert: false,
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
                    }
                );
        },
    },
});
</script>
