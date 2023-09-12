<template>
    <BootstrapAlert :show="!hasValidData">
        <h4 class="alert-heading">
            <BIconInfoSquare class="fs-2" />&nbsp;{{ $t('gridprofile.NoInfo') }}
        </h4>{{ $t('gridprofile.NoInfoLong') }}
    </BootstrapAlert>

    <template v-if="hasValidData">
        <BootstrapAlert :show="true" variant="danger">
            <h4 class="info-heading">
                <BIconInfoSquare class="fs-2" />&nbsp;{{ $t('gridprofile.GridprofileSupport') }}
            </h4><div v-html="$t('gridprofile.GridprofileSupportLong')"></div>
        </BootstrapAlert>
        <samp >
            {{ rawContent() }}
        </samp>
    </template>

</template>

<script lang="ts">
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import type { GridProfileStatus } from "@/types/GridProfileStatus";
import { BIconInfoSquare } from 'bootstrap-icons-vue';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        BootstrapAlert,
        BIconInfoSquare,
    },
    props: {
        gridProfileList: { type: Object as PropType<GridProfileStatus>, required: true },
    },
    computed: {
        rawContent() {
            return () => {
                return this.gridProfileList.raw.map(function (x) {
                    let y = x.toString(16); // to hex
                    y = ("00" + y).substr(-2); // zero-pad to 2-digits
                    return y
                }).join(' ');
            }
        },
        hasValidData() {
            return this.gridProfileList.raw.reduce((sum, x) => sum + x, 0) > 0;
        },
    },
});
</script>