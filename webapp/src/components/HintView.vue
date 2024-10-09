<template>
    <BootstrapAlert :show="hints.radio_problem" variant="danger">
        <BIconBroadcast class="fs-4" /> {{ $t('hints.RadioProblem') }}
    </BootstrapAlert>

    <BootstrapAlert :show="hints.time_sync" variant="danger">
        <BIconClock class="fs-4" /> {{ $t('hints.TimeSync') }}
        <a @click="gotoTimeSettings" href="#" class="alert-link">{{ $t('hints.TimeSyncLink') }}</a>
    </BootstrapAlert>

    <BootstrapAlert :show="hints.default_password" variant="danger">
        <BIconExclamationCircle class="fs-4" /> {{ $t('hints.DefaultPassword') }}
        <a @click="gotoPasswordSettings" href="#" class="alert-link">{{ $t('hints.DefaultPasswordLink') }}</a>
    </BootstrapAlert>
</template>

<script lang="ts">
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import type { Hints } from '@/types/LiveDataStatus';
import { BIconBroadcast, BIconClock, BIconExclamationCircle } from 'bootstrap-icons-vue';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        BootstrapAlert,
        BIconBroadcast,
        BIconClock,
        BIconExclamationCircle,
    },
    props: {
        hints: { type: Object as PropType<Hints>, required: true },
    },
    methods: {
        gotoTimeSettings() {
            this.$router.push('/settings/ntp');
        },
        gotoPasswordSettings() {
            this.$router.push('/settings/security');
        },
    },
});
</script>
