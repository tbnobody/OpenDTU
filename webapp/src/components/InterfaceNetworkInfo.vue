<template>
    <CardElement
        :text="$t('interfacenetworkinfo.NetworkInterface', { iface: networkStatus.network_mode })"
        textVariant="text-bg-primary"
        table
    >
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('interfacenetworkinfo.Hostname') }}</th>
                        <td>{{ networkStatus.network_hostname }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('interfacenetworkinfo.IpAddress') }}</th>
                        <td>{{ networkStatus.network_ip }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('interfacenetworkinfo.Netmask') }}</th>
                        <td>{{ networkStatus.network_netmask }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('interfacenetworkinfo.DefaultGateway') }}</th>
                        <td>{{ networkStatus.network_gateway }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('interfacenetworkinfo.Dns', { num: 1 }) }}</th>
                        <td>{{ networkStatus.network_dns1 }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('interfacenetworkinfo.Dns', { num: 2 }) }}</th>
                        <td>{{ networkStatus.network_dns2 }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('interfacenetworkinfo.MacAddress') }}</th>
                        <td>{{ networkStatus.network_mac }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import type { NetworkStatus } from '@/types/NetworkStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        networkStatus: { type: Object as PropType<NetworkStatus>, required: true },
    },
});
</script>
