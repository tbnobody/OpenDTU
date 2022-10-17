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
                            <td>{{ ap_ip }}</td>
                        </tr>
                        <tr>
                            <th>MAC Address</th>
                            <td>{{ ap_mac }}</td>
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
        ap_ip: String,
        ap_mac: String,
    },
});
</script>
