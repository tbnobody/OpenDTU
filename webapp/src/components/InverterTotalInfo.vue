<template>
    <div class="row row-cols-1 row-cols-md-3 g-3">
        <div class="col">
            <div class="card">
                <div class="card-header text-bg-success">{{ $t('invertertotalinfo.TotalYieldTotal') }}</div>
                <div class="card-body card-text text-center">
                    <h2>{{ formatNumber(totalData.YieldTotal.v, totalData.YieldTotal.d) }}
                        <small class="text-muted">{{ totalData.YieldTotal.u }}</small>
                    </h2>
                </div>
            </div>
        </div>
        <div class="col">
            <div class="card">
                <div class="card-header text-bg-success">{{ $t('invertertotalinfo.TotalYieldDay') }}</div>
                <div class="card-body card-text text-center">
                    <h2>{{ formatNumber(totalData.YieldDay.v, totalData.YieldDay.d) }}
                        <small class="text-muted">{{ totalData.YieldDay.u }}</small>
                    </h2>
                </div>
            </div>
        </div>
        <div class="col">
            <div class="card">
                <div class="card-header text-bg-success">{{ $t('invertertotalinfo.TotalPower') }}</div>
                <div class="card-body card-text text-center">
                    <h2>{{ formatNumber(totalData.Power.v, totalData.Power.d) }}
                        <small class="text-muted">{{ totalData.Power.u }}</small>
                    </h2>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, type PropType } from 'vue';
import type { Total } from '@/types/LiveDataStatus';
import { formatNumber } from '@/utils';

export default defineComponent({
    props: {
        totalData: { type: Object as PropType<Total>, required: true },
    },
    methods: {
        formatNumber,
    },
});
</script>