<template>
    <BasePage :title="$t('wireguardinfo.WireguardInformation')" :isLoading="dataLoading">
        <CardElement :text="$t('wireguardinfo.ConfigurationSummary')" textVariant="text-bg-primary">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>{{ $t('wireguardinfo.EndpointAddress') }}</th>
                            <td>{{ wireguardDataList.wg_endpoint_address }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('wireguardinfo.EndpointPort') }}</th>
                            <td>{{ wireguardDataList.wg_endpoint_port }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('wireguardinfo.EndpointLocalIP') }}</th>
                            <td>{{ wireguardDataList.wg_endpoint_local_ip }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('wireguardinfo.EndpointPublicKey') }}</th>
                            <td>{{ wireguardDataList.wg_endpoint_public_key }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('wireguardinfo.OpenDTULocalIP') }}</th>
                            <td>{{ wireguardDataList.wg_opendtu_local_ip }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('wireguardinfo.OpenDTUPublicKey') }}</th>
                            <td>{{ wireguardDataList.wg_opendtu_public_key }}</td>
                        </tr>
                        <tr>
                            <th>{{ $t('wireguardinfo.Status') }}</th>
                            <td class="badge" :class="{
                                'text-bg-danger': !wireguardDataList.wg_enabled,
                                'text-bg-success': wireguardDataList.wg_enabled,
                            }">
                                <span v-if="wireguardDataList.wg_enabled">{{ $t('wireguardinfo.Enabled') }}</span>
                                <span v-else>{{ $t('wireguardinfo.Disabled') }}</span>
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
import type { WireguardStatus } from "@/types/WireguardStatus";
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
            wireguardDataList: {} as WireguardStatus,
        };
    },
    created() {
        this.getWireguardInfo();
    },
    methods: {
        getWireguardInfo() {
            this.dataLoading = true;
            fetch("/api/wireguard/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.wireguardDataList = data;
                    this.dataLoading = false;
                });
        },
    },
});
</script>