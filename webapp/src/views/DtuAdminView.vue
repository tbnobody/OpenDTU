<template>
    <BasePage :title="$t('dtuadmin.DtuSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveDtuConfig">
            <CardElement :text="$t('dtuadmin.DtuConfiguration')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('dtuadmin.Serial')"
                    v-model="dtuConfigList.serial"
                    type="number"
                    min="1"
                    max="199999999999"
                    :tooltip="$t('dtuadmin.SerialHint')"
                />

                <InputElement
                    :label="$t('dtuadmin.PollInterval')"
                    v-model="dtuConfigList.pollinterval"
                    type="number"
                    min="1"
                    max="86400"
                    :postfix="$t('dtuadmin.Seconds')"
                />

                <div class="row mb-3" v-if="dtuConfigList.nrf_enabled">
                    <label for="inputNrfPaLevel" class="col-sm-2 col-form-label">
                        {{ $t('dtuadmin.NrfPaLevel') }}
                        <BIconInfoCircle v-tooltip :title="$t('dtuadmin.NrfPaLevelHint')" />
                    </label>
                    <div class="col-sm-10">
                        <select id="inputNrfPaLevel" class="form-select" v-model="dtuConfigList.nrf_palevel">
                            <option v-for="palevel in nrfpalevelList" :key="palevel.key" :value="palevel.key">
                                {{ $t(`dtuadmin.` + palevel.value, { db: palevel.db }) }}
                            </option>
                        </select>
                    </div>
                </div>

                <div class="row mb-3" v-if="dtuConfigList.cmt_enabled">
                    <label for="inputCmtPaLevel" class="col-sm-2 col-form-label">
                        {{ $t('dtuadmin.CmtPaLevel') }}
                        <BIconInfoCircle v-tooltip :title="$t('dtuadmin.CmtPaLevelHint')" />
                    </label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input
                                type="range"
                                class="form-control form-range"
                                v-model.number="dtuConfigList.cmt_palevel"
                                min="-10"
                                max="20"
                                id="inputCmtPaLevel"
                                aria-describedby="basic-addon1"
                                style="height: unset"
                            />
                            <span class="input-group-text" id="basic-addon1">{{ cmtPaLevelText }}</span>
                        </div>
                    </div>
                </div>

                <div class="row mb-3" v-if="dtuConfigList.cmt_enabled">
                    <label for="inputCmtCountry" class="col-sm-2 col-form-label">
                        {{ $t('dtuadmin.CmtCountry') }}
                        <BIconInfoCircle v-tooltip :title="$t('dtuadmin.CmtCountryHint')" />
                    </label>
                    <div class="col-sm-10">
                        <select id="inputCmtCountry" class="form-select" v-model="dtuConfigList.cmt_country">
                            <option v-for="(country, index) in dtuConfigList.country_def" :key="index" :value="index">
                                {{
                                    $t(`dtuadmin.country_` + index, {
                                        min: country.freq_min / 1e6,
                                        max: country.freq_max / 1e6,
                                    })
                                }}
                            </option>
                        </select>
                    </div>
                </div>

                <div class="row mb-3" v-if="dtuConfigList.cmt_enabled">
                    <label for="cmtFrequency" class="col-sm-2 col-form-label">
                        {{ $t('dtuadmin.CmtFrequency') }}
                        <BIconInfoCircle v-tooltip :title="$t('dtuadmin.CmtFrequencyHint')" />
                    </label>
                    <div class="col-sm-10">
                        <div class="input-group">
                            <input
                                type="range"
                                class="form-control form-range"
                                v-model.number="dtuConfigList.cmt_frequency"
                                :min="cmtMinFrequency"
                                :max="cmtMaxFrequency"
                                :step="dtuConfigList.cmt_chan_width"
                                id="cmtFrequency"
                                aria-describedby="basic-addon2"
                                style="height: unset"
                            />
                            <span class="input-group-text" id="basic-addon2">{{ cmtFrequencyText }}</span>
                        </div>
                        <div
                            class="alert alert-danger"
                            role="alert"
                            v-html="$t('dtuadmin.CmtFrequencyWarning')"
                            v-if="cmtIsOutOfLegalRange"
                        ></div>
                    </div>
                </div>
            </CardElement>
            <FormFooter @reload="getDtuConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { DtuConfig } from '@/types/DtuConfig';
import { authHeader, handleResponse } from '@/utils/authentication';
import { BIconInfoCircle } from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

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
            dtuConfigList: {} as DtuConfig,
            nrfpalevelList: [
                { key: 0, value: 'Min', db: '-18' },
                { key: 1, value: 'Low', db: '-12' },
                { key: 2, value: 'High', db: '-6' },
                { key: 3, value: 'Max', db: '0' },
            ],
            alertMessage: '',
            alertType: 'info',
            showAlert: false,
        };
    },
    created() {
        this.getDtuConfig();
    },
    computed: {
        cmtFrequencyText() {
            return this.$t('dtuadmin.MHz', {
                mhz: this.$n(this.dtuConfigList.cmt_frequency / 1000000, 'decimalTwoDigits'),
            });
        },
        cmtPaLevelText() {
            return this.$t('dtuadmin.dBm', { dbm: this.$n(this.dtuConfigList.cmt_palevel * 1) });
        },
        cmtMinFrequency() {
            return this.dtuConfigList.country_def[this.dtuConfigList.cmt_country].freq_min;
        },
        cmtMaxFrequency() {
            return this.dtuConfigList.country_def[this.dtuConfigList.cmt_country].freq_max;
        },
        cmtIsOutOfLegalRange() {
            return (
                this.dtuConfigList.cmt_frequency <
                    this.dtuConfigList.country_def[this.dtuConfigList.cmt_country].freq_legal_min ||
                this.dtuConfigList.cmt_frequency >
                    this.dtuConfigList.country_def[this.dtuConfigList.cmt_country].freq_legal_max
            );
        },
    },
    watch: {
        'dtuConfigList.cmt_country'(newValue, oldValue) {
            // Don't do anything on initial load (then oldValue equals undefined)
            if (oldValue != undefined) {
                this.$nextTick(() => {
                    this.dtuConfigList.cmt_frequency = this.dtuConfigList.country_def[newValue].freq_default;
                });
            }
        },
    },
    methods: {
        getDtuConfig() {
            this.dataLoading = true;
            fetch('/api/dtu/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.dtuConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveDtuConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.dtuConfigList));

            fetch('/api/dtu/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                    this.alertType = response.type;
                    this.showAlert = true;
                });
        },
    },
});
</script>
