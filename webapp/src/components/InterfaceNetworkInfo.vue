<template>
    <div class="card">
        <div class="card-header text-bg-primary">
            Network Interface ({{ networkStatus.network_mode }})
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Hostname</th>
                            <td>{{ networkStatus.network_hostname }}</td>
                        </tr>
                        <tr>
                            <th>IP Address</th>
                            <td>{{ networkStatus.network_ip }}</td>
                        </tr>
                        <tr>
                            <th>Netmask</th>
                            <td>{{ networkStatus.network_netmask }}</td>
                        </tr>
                        <tr>
                            <th>Default Gateway</th>
                            <td>{{ networkStatus.network_gateway }}</td>
                        </tr>
                        <tr>
                            <th>DNS 1</th>
                            <td>{{ networkStatus.network_dns1 }}</td>
                        </tr>
                        <tr>
                            <th>DNS 2</th>
                            <td>{{ networkStatus.network_dns2 }}</td>
                        </tr>
                        <tr>
                            <th>MAC Address</th>
                            <td>{{ networkStatus.network_mac }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import type { NetworkStatus } from '@/types/NetworkStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    props: {
        networkStatus: { type: Object as PropType<NetworkStatus>, required: true },
    },
});
</script>
