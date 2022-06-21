<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            Network Interface (Access Point)
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>IP Address</th>
                            <td>{{ networkDataList.ap_ip }}</td>
                        </tr>
                        <tr>
                            <th>MAC Address</th>
                            <td>{{ networkDataList.ap_mac }}</td>
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
