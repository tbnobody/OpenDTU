<template>
    <CardElement :text="$t('interfaceapinfo.NetworkInterface')" textVariant="text-bg-primary">
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('interfaceapinfo.IpAddress') }}</th>
                        <td>{{ networkStatus.ap_ip }}</td>
                    </tr>
                    <tr>
                        <th>{{ $t('interfaceapinfo.MacAddress') }}</th>
                        <td>{{ networkStatus.ap_mac }}</td>
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
