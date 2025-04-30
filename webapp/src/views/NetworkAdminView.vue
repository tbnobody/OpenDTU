<template>
    <BasePage :title="$t('networkadmin.NetworkSettings')" :isLoading="dataLoading">
        <BootstrapAlert
            v-model="alert.show"
            dismissible
            :variant="alert.type"
            :auto-dismiss="alert.type != 'success' ? 0 : 5000"
        >
            {{ alert.message }}
        </BootstrapAlert>

        <form @submit="saveNetworkConfig">
            <CardElement :text="$t('networkadmin.WifiConfiguration')" textVariant="text-bg-primary">
                <InputElement
                    :label="$t('networkadmin.WifiSsid')"
                    v-model="networkConfigList.ssid"
                    type="text"
                    maxlength="32"
                />

                <InputElement
                    :label="$t('networkadmin.WifiPassword')"
                    v-model="networkConfigList.password"
                    type="password"
                    maxlength="64"
                />

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
    },
    data() {
        return {
            dataLoading: true,
            networkConfigList: {} as NetworkConfig,
            alert: {} as AlertResponse,
        };
    },
    created() {
        this.getNetworkConfig();
    },
    methods: {
        getNetworkConfig() {
            this.dataLoading = true;
            fetch('/api/network/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.networkConfigList = data;
                    this.dataLoading = false;
                });
        },
        saveNetworkConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.networkConfigList));

            fetch('/api/network/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    this.alert.message = this.$t('apiresponse.' + response.code, response.param);
                    this.alert.type = response.type;
                    this.alert.show = true;
                });
        },
    },
});
</script>
