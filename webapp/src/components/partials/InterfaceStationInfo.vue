<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            Network Interface (Station)
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>IP Address</th>
                            <td>{{ sta_ip }}</td>
                        </tr>
                        <tr>
                            <th>Netmask</th>
                            <td>{{ sta_netmask }}</td>
                        </tr>
                        <tr>
                            <th>Default Gateway</th>
                            <td>{{ sta_gateway }}</td>
                        </tr>
                        <tr>
                            <th>DNS 1</th>
                            <td>{{ sta_dns1 }}</td>
                        </tr>
                        <tr>
                            <th>DNS 2</th>
                            <td>{{ sta_dns2 }}</td>
                        </tr>
                        <tr>
                            <th>MAC Address</th>
                            <td>{{ sta_mac }}</td>
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
        sta_ip: String,
        sta_netmask: String,
        sta_gateway: String,
        sta_dns1: String,
        sta_dns2: String,
        sta_mac: String,
    },
});
</script>
