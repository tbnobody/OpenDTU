<template>
    <BootstrapAlert :show="hints.radio_problem" variant="danger">
        <BIconBroadcast class="fs-4" /> Could not connect to a correct NRF24L01+ radio module. Please check the wiring.
    </BootstrapAlert>

    <BootstrapAlert :show="hints.time_sync" variant="danger">
        <BIconClock class="fs-4" /> The clock has not yet been synchronised. Without a
        correctly set clock, no requests are made to the inverter. This is normal shortly after the start. However,
        after a longer runtime (>1 minute), it indicates that the NTP server is not accessible. <a
            @click="gotoTimeSettings" href="#" class="alert-link">Please check your time
            settings.</a>
    </BootstrapAlert>

    <BootstrapAlert :show="hints.default_password" variant="danger">
        <BIconExclamationCircle class="fs-4" /> You are using the default password for the web interface and the
        emergency access point. This is potentially insecure. <a @click="gotoPasswordSettings" href="#"
            class="alert-link">Please change the password.</a>
    </BootstrapAlert>
</template>

<script lang="ts">
import { defineComponent, type PropType } from 'vue';
import type { Hints } from '@/types/LiveDataStatus';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import {
    BIconClock,
    BIconExclamationCircle,
    BIconBroadcast,
} from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BootstrapAlert,
        BIconClock,
        BIconExclamationCircle,
        BIconBroadcast,
    },
    props: {
        hints: { type: Object as PropType<Hints>, required: true },
    },
    methods: {
        gotoTimeSettings() {
            this.$router.push("/settings/ntp");
        },
        gotoPasswordSettings() {
            this.$router.push("/settings/security");
        }
    }
});
</script>