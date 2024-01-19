<template>
    <BootstrapAlert :show="!hasValidData">
        <h4 class="alert-heading">
            <BIconInfoSquare class="fs-2" />&nbsp;{{ $t('gridprofile.NoInfo') }}
        </h4>{{ $t('gridprofile.NoInfoLong') }}
    </BootstrapAlert>

    <template v-if="hasValidData">
        <table class="table table-hover">
        <tbody>
            <tr>
                <td>{{ $t('gridprofile.Name') }}</td>
                <td>{{ gridProfileList.name }}</td>
            </tr>
            <tr>
                <td>{{ $t('gridprofile.Version') }}</td>
                <td>{{ gridProfileList.version }}</td>
            </tr>
        </tbody>
        </table>

        <div class="accordion" id="accordionProfile">
            <div class="accordion-item" v-for="(section, index) in gridProfileList.sections" :key="index">
                <h2 class="accordion-header">
                    <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" :data-bs-target="`#collapse${index}`" aria-expanded="true" :aria-controls="`collapse${index}`">
                        {{ section.name }}
                    </button>
                </h2>
                <div :id="`collapse${index}`" class="accordion-collapse collapse" data-bs-parent="#accordionProfile">
                    <div class="accordion-body">
                        <table class="table table-hover">
                        <tbody>
                            <tr v-for="value in section.items" :key="value.n">
                                <th>{{ value.n }}</th>
                                <td>
                                    <template v-if="value.u!='bool'">
                                        {{ $n(value.v, 'decimal') }} {{ value.u }}
                                    </template>
                                    <template v-else>
                                        <StatusBadge :status="value.v==1" true_text="gridprofile.Enabled" false_text="gridprofile.Disabled"/>
                                    </template>
                                </td>
                            </tr>
                        </tbody>
                        </table>
                    </div>
                </div>
            </div>
        </div>

        <br />

        <div class="accordion" id="accordionDev">
            <div class="accordion-item">
                <h2 class="accordion-header">
                    <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#collapseDev" aria-expanded="true" aria-controls="collapseDev">
                        {{ $t('gridprofile.GridprofileSupport') }}
                    </button>
                </h2>
                <div id="collapseDev" class="accordion-collapse collapse" data-bs-parent="#accordionDev">
                    <div class="accordion-body">
                        <BootstrapAlert :show="true" variant="danger">
                            <h4 class="info-heading">
                                <BIconInfoSquare class="fs-2" />&nbsp;{{ $t('gridprofile.GridprofileSupport') }}
                            </h4><div v-html="$t('gridprofile.GridprofileSupportLong')"></div>
                        </BootstrapAlert>
                        <samp>
                            {{ rawContent() }}
                        </samp>
                    </div>
                </div>
            </div>
        </div>

    </template>

</template>

<script lang="ts">
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import type { GridProfileRawdata } from '@/types/GridProfileRawdata';
import type { GridProfileStatus } from "@/types/GridProfileStatus";
import { BIconInfoSquare } from 'bootstrap-icons-vue';
import { defineComponent, type PropType } from 'vue';
import StatusBadge from './StatusBadge.vue';

export default defineComponent({
    components: {
        BootstrapAlert,
        BIconInfoSquare,
        StatusBadge,
    },
    props: {
        gridProfileList: { type: Object as PropType<GridProfileStatus>, required: true },
        gridProfileRawList: { type: Object as PropType<GridProfileRawdata>, required: true },
    },
    computed: {
        rawContent() {
            return () => {
                return this.gridProfileRawList.raw.map(function (x) {
                    let y = x.toString(16); // to hex
                    y = ("00" + y).substr(-2); // zero-pad to 2-digits
                    return y
                }).join(' ');
            }
        },
        hasValidData() {
            return this.gridProfileRawList.raw.reduce((sum, x) => sum + x, 0) > 0;
        },
    },
});
</script>
