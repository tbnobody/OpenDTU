<template>
    <BasePage
        :title="$t('networkadmin.NetworkSettings')"
        :isLoading="dataLoading"
        show-reload
        @reload="getNetworkConfig"
    >
        <BootstrapAlert
            v-model="alert.show"
            dismissible
            :variant="alert.type"
            :auto-dismiss="alert.type != 'success' ? 0 : 5000"
        >
            {{ alert.message }}
        </BootstrapAlert>

        <NetworkConnectionStatus class="mb-4" />

        <form v-if="configLoaded" @submit="saveNetworkConfig" :aria-busy="saving">
            <CardElement :text="$t('networkadmin.WifiConfiguration')" textVariant="text-bg-primary">
                <WifiNetworkScan v-model="networkConfigList.ssid" :disabled="saving" />
                <InputElement
                    :label="$t('networkadmin.WifiSsid')"
                    v-model="networkConfigList.ssid"
                    type="text"
                    maxlength="32"
                />

                <InputElement
                    :label="$t('networkadmin.WifiPassword')"
                    v-model="networkConfigList.password"
                    :type="showPassword ? 'text' : 'password'"
                    maxlength="64"
                >
                    <button
                        type="button"
                        class="btn btn-outline-secondary btn-sm mt-2"
                        :aria-pressed="showPassword"
                        @click="showPassword = !showPassword"
                    >
                        {{ $t(showPassword ? 'networkadmin.HidePassword' : 'networkadmin.ShowPassword') }}
                    </button>
                </InputElement>

                <InputElement
                    :label="$t('networkadmin.Hostname')"
                    v-model="networkConfigList.hostname"
                    type="text"
                    maxlength="32"
                >
                    <div
                        class="alert alert-secondary mb-0 mt-3"
                        role="alert"
                        v-html="$t('networkadmin.HostnameHint')"
                    ></div>
                </InputElement>

                <InputElement :label="$t('networkadmin.EnableDhcp')" v-model="networkConfigList.dhcp" type="checkbox" />
            </CardElement>

            <CardElement
                :text="$t('networkadmin.StaticIpConfiguration')"
                textVariant="text-bg-primary"
                add-space
                v-show="!networkConfigList.dhcp"
            >
                <InputElement
                    :label="$t('networkadmin.IpAddress')"
                    v-model="networkConfigList.ipaddress"
                    type="text"
                    maxlength="32"
                />

                <InputElement
                    :label="$t('networkadmin.Netmask')"
                    v-model="networkConfigList.netmask"
                    type="text"
                    maxlength="32"
                />

                <InputElement
                    :label="$t('networkadmin.DefaultGateway')"
                    v-model="networkConfigList.gateway"
                    type="text"
                    maxlength="32"
                />

                <InputElement
                    :label="$t('networkadmin.Dns', { num: 1 })"
                    v-model="networkConfigList.dns1"
                    type="text"
                    maxlength="32"
                />

                <InputElement
                    :label="$t('networkadmin.Dns', { num: 2 })"
                    v-model="networkConfigList.dns2"
                    type="text"
                    maxlength="32"
                />
            </CardElement>

            <CardElement :text="$t('networkadmin.MdnsSettings')" textVariant="text-bg-primary" add-space>
                <InputElement
                    :label="$t('networkadmin.EnableMdns')"
                    v-model="networkConfigList.mdnsenabled"
                    type="checkbox"
                />
            </CardElement>

            <CardElement :text="$t('networkadmin.SyslogSettings')" textVariant="text-bg-primary" add-space>
                <InputElement
                    :label="$t('networkadmin.EnableSyslog')"
                    v-model="networkConfigList.syslogenabled"
                    type="checkbox"
                />

                <div v-if="networkConfigList.syslogenabled">
                    <InputElement
                        :label="$t('networkadmin.SyslogHostname')"
                        v-model="networkConfigList.sysloghostname"
                        type="text"
                        maxlength="128"
                    />

                    <InputElement
                        :label="$t('networkadmin.SyslogPort')"
                        v-model="networkConfigList.syslogport"
                        type="number"
                        min="1"
                        max="65535"
                    />
                </div>
            </CardElement>

            <CardElement :text="$t('networkadmin.AdminAp')" textVariant="text-bg-primary" add-space>
                <p>{{ $t('networkadmin.ApCoexistenceHint') }}</p>
                <p>{{ $t('networkadmin.ReconnectHint') }}</p>
                <InputElement
                    :label="$t('networkadmin.ApTimeout')"
                    v-model="networkConfigList.aptimeout"
                    type="number"
                    min="0"
                    max="99999"
                    :postfix="$t('networkadmin.Minutes')"
                    :tooltip="$t('networkadmin.ApTimeoutHint')"
                />
            </CardElement>
            <p v-if="saving" class="mt-3" role="status">{{ $t('networkadmin.Saving') }}</p>
            <FormFooter @reload="getNetworkConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import NetworkConnectionStatus from '@/components/NetworkConnectionStatus.vue';
import WifiNetworkScan from '@/components/WifiNetworkScan.vue';
import type { AlertResponse } from '@/types/AlertResponse';
import type { NetworkConfig } from '@/types/NetworkConfig';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
        InputElement,
        NetworkConnectionStatus,
        WifiNetworkScan,
    },
    data() {
        return {
            dataLoading: true,
            configLoaded: false,
            showPassword: false,
            saving: false,
            active: true,
            configController: null as AbortController | null,
            networkConfigList: {} as NetworkConfig,
            alert: {} as AlertResponse,
        };
    },
    created() {
        this.getNetworkConfig();
    },
    beforeUnmount() {
        this.active = false;
        this.showPassword = false;
        this.configController?.abort();
    },
    methods: {
        async requestConfig(method: 'GET' | 'POST', body?: FormData) {
            const controller = new AbortController();
            this.configController = controller;
            const timeout = window.setTimeout(() => controller.abort(), 10000);
            try {
                const options: RequestInit = {
                    method,
                    headers: authHeader(),
                    signal: controller.signal,
                    cache: 'no-store',
                };
                if (method === 'POST') {
                    options.body = body;
                }
                const response = await fetch('/api/network/config', options);
                return await handleResponse(response, this.$emitter, this.$router, true);
            } finally {
                window.clearTimeout(timeout);
                this.configController = null;
            }
        },
        async getNetworkConfig() {
            this.showPassword = false;
            if (this.configController) return;
            this.dataLoading = true;
            try {
                const data = await this.requestConfig('GET');
                if (this.active) {
                    this.networkConfigList = data;
                    this.configLoaded = true;
                    this.alert.show = false;
                }
            } catch {
                if (this.active) this.showRequestError('networkadmin.LoadFailed');
            } finally {
                if (this.active) this.dataLoading = false;
            }
        },
        async saveNetworkConfig(e: Event) {
            e.preventDefault();
            this.showPassword = false;
            if (this.configController) return;
            this.saving = true;
            this.alert.show = false;

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.networkConfigList));

            try {
                const response = await this.requestConfig('POST', formData);
                if (this.active) {
                    this.alert.message = this.$t('apiresponse.' + response.code, response.param);
                    this.alert.type = response.type;
                    this.alert.show = true;
                }
            } catch {
                if (this.active) this.showRequestError('networkadmin.SaveUnconfirmed');
            } finally {
                if (this.active) this.saving = false;
            }
        },
        showRequestError(key: string) {
            this.alert.message = this.$t(key);
            this.alert.type = 'warning';
            this.alert.show = true;
        },
    },
});
</script>
