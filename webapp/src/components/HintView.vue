<template>
    <BootstrapAlert :show="hints.radio_problem" variant="danger">
        <div class="d-flex">
            <div class="align-content-center"><BIconBroadcast class="fs-4" /></div>
            <div class="align-content-center ms-3">{{ $t('hints.RadioProblem') }}</div>
        </div>
    </BootstrapAlert>

    <BootstrapAlert :show="hints.time_sync" variant="danger">
        <div class="d-flex">
            <div class="align-content-center"><BIconClock class="fs-4" /></div>
            <div class="align-content-center ms-3">
                {{ $t('hints.TimeSync') }}
                <a @click="gotoTimeSettings" href="#" class="alert-link">{{ $t('hints.TimeSyncLink') }}</a>
            </div>
        </div>
    </BootstrapAlert>

    <BootstrapAlert :show="hints.default_password" variant="danger">
        <div class="d-flex">
            <div class="align-content-center"><BIconExclamationCircle class="fs-4" /></div>
            <div class="align-content-center ms-3">
                {{ $t('hints.DefaultPassword') }}
                <a @click="gotoPasswordSettings" href="#" class="alert-link">{{ $t('hints.DefaultPasswordLink') }}</a>
            </div>
        </div>
    </BootstrapAlert>

    <BootstrapAlert :show="hints.pin_mapping_issue" variant="warning">
        <div class="d-flex">
            <div class="align-content-center"><BIconExclamationCircle class="fs-4" /></div>
            <div class="align-content-center ms-3" v-html="$t('hints.PinMappingIssue')"></div>
        </div>
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
