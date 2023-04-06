<template>
  <div v-show="totalVeData.enabled">
    <div class="row row-cols-1 row-cols-md-3 g-3">
      <div class="col">
        <div class="card">
          <div class="card-header text-bg-success">{{ $t('invertertotalinfo.MpptTotalYieldTotal') }}</div>
          <div class="card-body card-text text-center">
            <h2>
              {{ $n(totalVeData.total.YieldTotal.v, 'decimal', {
                minimumFractionDigits: totalVeData.total.YieldTotal.d,
                maximumFractionDigits: totalVeData.total.YieldTotal.d
              }) }}
              <small class="text-muted">{{ totalVeData.total.YieldTotal.u }}</small>
            </h2>
          </div>
        </div>
      </div>
      <div class="col">
        <div class="card">
          <div class="card-header text-bg-success">{{ $t('invertertotalinfo.MpptTotalYieldDay') }}</div>
          <div class="card-body card-text text-center">
            <h2>
              {{ $n(totalVeData.total.YieldDay.v, 'decimal', {
                minimumFractionDigits: totalVeData.total.YieldDay.d,
                maximumFractionDigits: totalVeData.total.YieldDay.d
              }) }}
              <small class="text-muted">{{ totalVeData.total.YieldDay.u }}</small>
            </h2>
          </div>
        </div>
      </div>
      <div class="col">
        <div class="card">
          <div class="card-header text-bg-success">{{ $t('invertertotalinfo.MpptTotalPower') }}</div>
          <div class="card-body card-text text-center">
            <h2>
              {{ $n(totalVeData.total.Power.v, 'decimal', {
                minimumFractionDigits: totalVeData.total.Power.d,
                maximumFractionDigits: totalVeData.total.Power.d
              }) }}
              <small class="text-muted">{{ totalVeData.total.Power.u }}</small>
            </h2>
          </div>
        </div>
      </div>
    </div>
  </div>
  <div class="row row-cols-1 row-cols-md-3 g-3">
    <div class="col">
      <div class="card">
        <div class="card-header text-bg-success">{{ $t('invertertotalinfo.InverterTotalYieldTotal') }}</div>
        <div class="card-body card-text text-center">
          <h2>
            {{ $n(totalData.YieldTotal.v, 'decimal', {
              minimumFractionDigits: totalData.YieldTotal.d,
              maximumFractionDigits: totalData.YieldTotal.d
            }) }}
            <small class="text-muted">{{ totalData.YieldTotal.u }}</small>
          </h2>
        </div>
      </div>
    </div>
    <div class="col">
      <div class="card">
        <div class="card-header text-bg-success">{{ $t('invertertotalinfo.InverterTotalYieldDay') }}</div>
        <div class="card-body card-text text-center">
          <h2>
            {{ $n(totalData.YieldDay.v, 'decimal', {
              minimumFractionDigits: totalData.YieldDay.d,
              maximumFractionDigits: totalData.YieldDay.d
            }) }}
            <small class="text-muted">{{ totalData.YieldDay.u }}</small>
          </h2>
        </div>
      </div>
    </div>
    <div class="col">
      <div class="card">
        <div class="card-header text-bg-success">{{ $t('invertertotalinfo.InverterTotalPower') }}</div>
        <div class="card-body card-text text-center">
          <h2>
            {{ $n(totalData.Power.v, 'decimal', {
              minimumFractionDigits: totalData.Power.d,
              maximumFractionDigits: totalData.Power.d
            }) }}
            <small class="text-muted">{{ totalData.Power.u }}</small>
          </h2>
        </div>
      </div>
    </div>
  </div>
  <div v-show="totalBattData.enabled">
    <div class="row row-cols-1 row-cols-md-3 g-3">
      <div class="col">
        <div class="card">
          <div class="card-header text-bg-success">{{ $t('invertertotalinfo.BatterySoc') }}</div>
          <div class="card-body card-text text-center">
            <h2>
              {{ $n(totalBattData.soc.v, 'decimal', {
                minimumFractionDigits: totalBattData.soc.d,
                maximumFractionDigits: totalBattData.soc.d
              }) }}
              <small class="text-muted">{{ totalBattData.soc.u }}</small>
            </h2>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import type { Battery, Total, Vedirect } from '@/types/LiveDataStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
  props: {
    totalData: { type: Object as PropType<Total>, required: true },
    totalVeData: { type: Object as PropType<Vedirect>, required: true },
    totalBattData: { type: Object as PropType<Battery>, required: true },
  },
});
</script>