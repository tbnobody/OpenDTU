<template>
    <div class="card">
        <div class="card-header text-bg-primary">
            Network Interface (Access Point)
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>IP Address</th>
                            <td>{{ networkStatus.ap_ip }}</td>
                        </tr>
                        <tr>
                            <th>MAC Address</th>
                            <td>{{ networkStatus.ap_mac }}</td>
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
