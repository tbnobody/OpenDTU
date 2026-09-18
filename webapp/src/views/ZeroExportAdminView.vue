<template>
    <BasePage :title="$t('zeroexportadmin.ZeroExportSettings')" :isLoading="dataLoading">
        <BootstrapAlert
            v-model="alert.show"
            dismissible
            :variant="alert.type"
            :auto-dismiss="alert.type != 'success' ? 0 : 5000"
        >
            {{ alert.message }}
        </BootstrapAlert>

        <form @submit="saveConfig">
            <CardElement :text="$t('zeroexportadmin.ZeroExportConfiguration')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('zeroexportadmin.EnableZeroExportStartup')"
                    v-model="cfg.enabled"
                    type="checkbox"
                    :tooltip="$t('zeroexportadmin.EnableZeroExportStartupHint')"
                    wide
                />

                <InputElement
                    :label="$t('zeroexportadmin.UpdateInterval')"
                    v-model="cfg.update_interval"
                    type="number"
                    min="5"
                    max="3600"
                    :postfix="$t('zeroexportadmin.Seconds')"
                    :tooltip="$t('zeroexportadmin.UpdateIntervalHint')"
                    wide
                />

                <InputElement
                    :label="$t('zeroexportadmin.SetPointStartup')"
                    v-model="cfg.setpoint"
                    type="number"
                    step="1"
                    :postfix="$t('zeroexportadmin.Watts')"
                    :tooltip="$t('zeroexportadmin.SetPointStartupHint')"
                    wide
                />

                <InputElement
                    :label="$t('zeroexportadmin.HomeMinimalConsumption')"
                    v-model="cfg.home_minimal_consumption"
                    type="number"
                    min="0"
                    step="1"
                    :postfix="$t('zeroexportadmin.Watts')"
                    :tooltip="$t('zeroexportadmin.HomeMinimalConsumptionHint')"
                    wide
                />

                <InputElement
                    :label="$t('zeroexportadmin.InverterMinimalPower')"
                    v-model="cfg.inverter_minimal_power"
                    type="number"
                    min="40"
                    step="1"
                    :postfix="$t('zeroexportadmin.Watts')"
                    :tooltip="$t('zeroexportadmin.InverterMinimalPowerHint')"
                    wide
                />
            </CardElement>

            <CardElement :text="$t('zeroexportadmin.GridSource')" textVariant="text-bg-primary" add-space>
                <div class="row mb-3">
                    <label for="inputSource" class="col-sm-4 col-form-label">
                        {{ $t('zeroexportadmin.Source') }}
                        <BIconInfoCircle v-tooltip :title="$t('zeroexportadmin.SourceHint')" />
                    </label>
                    <div class="col-sm-8">
                        <select id="inputSource" class="form-select" v-model="cfg.source">
                            <option v-for="source in sourceList" :key="source.key" :value="source.key">
                                {{ $t(`zeroexportadmin.` + source.value) }}
                            </option>
                        </select>
                    </div>
                </div>

                <div class="border rounded p-3" v-if="cfg.source == 1">
                    <h5>{{ $t('zeroexportadmin.ShellyLnmConfiguration') }}</h5>
                    <InputElement
                        :label="$t('zeroexportadmin.ShellyLnmAddr')"
                        v-model="cfg.shelly_lnm_addr"
                        type="text"
                        maxlength="15"
                        :tooltip="$t('zeroexportadmin.ShellyLnmAddrHint')"
                        wide
                    />
                    <InputElement
                        :label="$t('zeroexportadmin.ShellyLnmPort')"
                        v-model="cfg.shelly_lnm_port"
                        type="number"
                        min="1"
                        max="65535"
                        wide
                    />
                    <div class="row mb-3">
                        <label for="inputShellyLnmType" class="col-sm-4 col-form-label">
                            {{ $t('zeroexportadmin.ShellyLnmType') }}
                        </label>
                        <div class="col-sm-8">
                            <select id="inputShellyLnmType" class="form-select" v-model="cfg.shelly_lnm_type">
                                <option value="act_power">Shelly EM (act_power)</option>
                                <option value="total_act_power">Shelly 3EM (total_act_power)</option>
                                <option value="a_act_power">Shelly 3EM - A (a_act_power)</option>
                                <option value="b_act_power">Shelly 3EM - B (b_act_power)</option>
                                <option value="c_act_power">Shelly 3EM - C (c_act_power)</option>
                            </select>
                        </div>
                    </div>
                    <InputElement
                        :label="$t('zeroexportadmin.FailsafeTimeout')"
                        v-model="cfg.shelly_lnm_failsafe_timeout"
                        type="number"
                        min="5"
                        max="300"
                        :postfix="$t('zeroexportadmin.Seconds')"
                        :tooltip="$t('zeroexportadmin.FailsafeTimeoutHint')"
                        wide
                    />
                </div>

                <div class="border rounded p-3" v-if="cfg.source == 2">
                    <h5>{{ $t('zeroexportadmin.MqttConfiguration') }}</h5>
                    <InputElement
                        :label="$t('zeroexportadmin.MqttGridPowerTopic')"
                        v-model="cfg.mqtt_grid_power_topic"
                        type="text"
                        maxlength="127"
                        :tooltip="$t('zeroexportadmin.MqttGridPowerTopicHint')"
                        wide
                    />
                    <div class="row mb-3">
                        <label for="inputMqttDataType" class="col-sm-4 col-form-label">
                            {{ $t('zeroexportadmin.MqttDataType') }}
                        </label>
                        <div class="col-sm-8">
                            <select id="inputMqttDataType" class="form-select" v-model="cfg.mqtt_data_type">
                                <option value="raw">{{ $t('zeroexportadmin.MqttDataTypeRaw') }}</option>
                                <option value="act_power">Shelly EM (act_power)</option>
                                <option value="total_act_power">Shelly 3EM (total_act_power)</option>
                                <option value="a_act_power">Shelly 3EM - A (a_act_power)</option>
                                <option value="b_act_power">Shelly 3EM - B (b_act_power)</option>
                                <option value="c_act_power">Shelly 3EM - C (c_act_power)</option>
                            </select>
                        </div>
                    </div>
                    <InputElement
                        :label="$t('zeroexportadmin.FailsafeTimeout')"
                        v-model="cfg.mqtt_failsafe_timeout"
                        type="number"
                        min="5"
                        max="300"
                        :postfix="$t('zeroexportadmin.Seconds')"
                        :tooltip="$t('zeroexportadmin.FailsafeTimeoutHint')"
                        wide
                    />
                </div>
            </CardElement>

            <FormFooter @reload="getConfig" />
        </form>

        <CardElement :text="$t('zeroexportadmin.ZeroExportStatus')" textVariant="text-bg-primary" add-space>
            <div class="row mb-3">
                <label class="col-sm-4 col-form-label">{{ $t('zeroexportadmin.ZeroExportRuntimeEnable') }}</label>
                <div class="col-sm-8">
                    <div class="form-check form-switch">
                        <input
                            class="form-check-input"
                            type="checkbox"
                            role="switch"
                            :checked="status.enabled"
                            disabled
                        />
                    </div>
                </div>
            </div>
            <div class="row mb-3">
                <label class="col-sm-4 col-form-label">{{ $t('zeroexportadmin.ZeroExportRuntimeSetPoint') }}</label>
                <div class="col-sm-8">
                    <input
                        class="form-control"
                        :value="status.setpoint == null ? $t('home.Pending') : status.setpoint + ' W'"
                        disabled
                    />
                </div>
            </div>
            <div class="row mb-3">
                <label class="col-sm-4 col-form-label">{{ $t('zeroexportadmin.GridPower') }}</label>
                <div class="col-sm-8">
                    <input
                        class="form-control"
                        :value="status.grid_power == null ? $t('home.Pending') : status.grid_power + ' W'"
                        disabled
                    />
                </div>
            </div>
            <div class="row mb-3">
                <label class="col-sm-4 col-form-label">{{ $t('zeroexportadmin.GridPowerAge') }}</label>
                <div class="col-sm-8">
                    <input
                        class="form-control"
                        :value="status.grid_power == null ? $t('home.Pending') : status.grid_power_age + ' s'"
                        disabled
                    />
                </div>
            </div>
            <div class="row mb-3">
                <label class="col-sm-4 col-form-label">
                    {{ $t('zeroexportadmin.GridUpdateInterval') }}
                    <BIconInfoCircle v-tooltip :title="$t('zeroexportadmin.GridUpdateIntervalHint')" />
                </label>
                <div class="col-sm-8">
                    <input
                        class="form-control"
                        :value="status.grid_update_period == 0 ? $t('home.Pending') : status.grid_update_period + ' ms'"
                        disabled
                    />
                </div>
            </div>
            <div class="row mb-3">
                <label class="col-sm-4 col-form-label">
                    {{ $t('zeroexportadmin.ProductionLimit') }}
                    <BIconInfoCircle v-tooltip :title="$t('zeroexportadmin.ProductionLimitHint')" />
                </label>
                <div class="col-sm-8">
                    <input
                        class="form-control"
                        :value="status.production_limit == 0 ? $t('home.Pending') : status.production_limit + ' W'"
                        disabled
                    />
                </div>
            </div>
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import type { AlertResponse } from '@/types/AlertResponse';
import type { ZeroExportConfig, ZeroExportStatus } from '@/types/ZeroExportConfig';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';
import { BIconInfoCircle } from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
        InputElement,
        BIconInfoCircle,
    },
    data() {
        return {
            dataLoading: true,
            statusLoading: false,
            cfg: {} as ZeroExportConfig,
            status: {} as ZeroExportStatus,
            alert: {} as AlertResponse,
            sourceList: [
                { key: 1, value: 'SourceShellyLnm' },
                { key: 2, value: 'SourceMqtt' },
            ],
            statusRefreshTimer: undefined as number | undefined,
        };
    },
    created() {
        this.getConfig();
        this.getStatus();
        this.statusRefreshTimer = window.setInterval(() => this.getStatus(), 5000);
    },
    beforeUnmount() {
        if (this.statusRefreshTimer !== undefined) {
            window.clearInterval(this.statusRefreshTimer);
        }
    },
    methods: {
        getConfig() {
            this.dataLoading = true;
            fetch('/api/zeroexport/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.cfg = data;
                    this.dataLoading = false;
                });
        },
        getStatus() {
            if (this.statusLoading) {
                return;
            }
            this.statusLoading = true;
            fetch('/api/zeroexport/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.status = data;
                    this.statusLoading = false;
                });
        },
        saveConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.cfg));

            fetch('/api/zeroexport/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    this.alert.message = this.$t('apiresponse.' + response.code, response.param);
                    this.alert.type = response.type;
                    this.alert.show = true;
                })
                .then(() => {
                    this.getConfig();
                });
        },
    },
});
</script>
