<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            Network Interface ({{ network_mode }})
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>IP Address</th>
                            <td>{{ network_ip }}</td>
                        </tr>
                        <tr>
                            <th>Netmask</th>
                            <td>{{ network_netmask }}</td>
                        </tr>
                        <tr>
                            <th>Default Gateway</th>
                            <td>{{ network_gateway }}</td>
                        </tr>
                        <tr>
                            <th>DNS 1</th>
                            <td>{{ network_dns1 }}</td>
                        </tr>
                        <tr>
                            <th>DNS 2</th>
                            <td>{{ network_dns2 }}</td>
                        </tr>
                        <tr>
                            <th>MAC Address</th>
                            <td>{{ network_mac }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    props: {
        network_ip: String,
        network_netmask: String,
        network_gateway: String,
        network_dns1: String,
        network_dns2: String,
        network_mac: String,
        network_mode: String,
    },
});
</script>
