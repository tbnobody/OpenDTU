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
                            <td>{{ networkDataList.sta_ip }}</td>
                        </tr>
                        <tr>
                            <th>Netmask</th>
                            <td>{{ networkDataList.sta_netmask }}</td>
                        </tr>
                        <tr>
                            <th>Default Gateway</th>
                            <td>{{ networkDataList.sta_gateway }}</td>
                        </tr>
                        <tr>
                            <th>DNS 1</th>
                            <td>{{ networkDataList.sta_dns1 }}</td>
                        </tr>
                        <tr>
                            <th>DNS 2</th>
                            <td>{{ networkDataList.sta_dns2 }}</td>
                        </tr>
                        <tr>
                            <th>MAC Address</th>
                            <td>{{ networkDataList.sta_mac }}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script>
import { defineComponent } from 'vue';

export default defineComponent({
    data() {
        return {
            networkDataList: [],
        };
    },
    created() {
        this.getNetworkInfo();
    },
    methods: {
        getNetworkInfo() {
            fetch("/api/network/status")
                .then((response) => response.json())
                .then((data) => (this.networkDataList = data));
        },
    },
});
</script>
