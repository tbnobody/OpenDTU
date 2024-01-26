<template>
  <div v-show="totalVeData.enabled">
    <div class="row row-cols-1 row-cols-md-3 g-3">
        <div class="col">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.MpptTotalYieldTotal')">
                <h2>
                    {{ $n(totalVeData.total.YieldTotal.v, 'decimal', {
                        minimumFractionDigits: totalVeData.total.YieldTotal.d,
                        maximumFractionDigits:totalVeData.total.YieldTotal.d
                    }) }}
                    <small class="text-muted">{{ totalVeData.total.YieldTotal.u }}</small>
                </h2>
            </CardElement>
        </div>
        <div class="col">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.MpptTotalYieldDay')">
                <h2>
                    {{ $n(totalVeData.total.YieldDay.v, 'decimal', {
                        minimumFractionDigits: totalVeData.total.YieldDay.d,
                        maximumFractionDigits: totalVeData.total.YieldDay.d
                    }) }}
                    <small class="text-muted">{{ totalVeData.total.YieldDay.u }}</small>
                </h2>
            </CardElement>
        </div>
        <div class="col">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.MpptTotalPower')">
                <h2>
                    {{ $n(totalVeData.total.Power.v, 'decimal', {
                        minimumFractionDigits: totalVeData.total.Power.d,
                        maximumFractionDigits: totalVeData.total.Power.d
                    }) }}
                    <small class="text-muted">{{ totalVeData.total.Power.u }}</small>
                </h2>
            </CardElement>
        </div>
    </div>
  </div>
  <div class="row row-cols-1 row-cols-md-3 g-3">
        <div class="col">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.InverterTotalYieldTotal')">
                <h2>
                    {{ $n(totalData.YieldTotal.v, 'decimal', {
                        minimumFractionDigits: totalData.YieldTotal.d,
                        maximumFractionDigits: totalData.YieldTotal.d
                    }) }}
                    <small class="text-muted">{{ totalData.YieldTotal.u }}</small>
                </h2>
            </CardElement>
        </div>
        <div class="col">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.InverterTotalYieldDay')">
                <h2>
                    {{ $n(totalData.YieldDay.v, 'decimal', {
                        minimumFractionDigits: totalData.YieldDay.d,
                        maximumFractionDigits: totalData.YieldDay.d
                    }) }}
                    <small class="text-muted">{{ totalData.YieldDay.u }}</small>
                </h2>
            </CardElement>
        </div>
        <div class="col">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.InverterTotalPower')">
                <h2>
                    {{ $n(totalData.Power.v, 'decimal', {
                        minimumFractionDigits: totalData.Power.d,
                        maximumFractionDigits: totalData.Power.d
                    }) }}
                    <small class="text-muted">{{ totalData.Power.u }}</small>
                </h2>
            </CardElement>
        </div>
  </div>
  <div v-show="totalBattData.enabled || powerMeterData.enabled || huaweiData.enabled">
    <div class="row row-cols-1 row-cols-md-3 g-3">
      <div class="col" v-show="totalBattData.enabled">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.BatterySoc')">
                <h2>
                    {{ $n(totalBattData.soc.v, 'decimal', {
                        minimumFractionDigits: totalBattData.soc.d,
                        maximumFractionDigits: totalBattData.soc.d
                    }) }}
                    <small class="text-muted">{{ totalBattData.soc.u }}</small>
                </h2>
            </CardElement>
      </div>
      <div class="col" v-show="powerMeterData.enabled">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.HomePower')">
                <h2>
                    {{ $n(powerMeterData.Power.v, 'decimal', {
                        minimumFractionDigits: powerMeterData.Power.d,
                        maximumFractionDigits: powerMeterData.Power.d
                    }) }}
                    <small class="text-muted">{{powerMeterData.Power.u }}</small>
                </h2>
            </CardElement>
      </div>
      <div class="col" v-show="huaweiData.enabled">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.HuaweiPower')">
                <h2>
                    {{ $n(huaweiData.Power.v, 'decimal', {
                        minimumFractionDigits: huaweiData.Power.d,
                        maximumFractionDigits: huaweiData.Power.d
                    }) }}
                    <small class="text-muted">{{huaweiData.Power.u }}</small>
                </h2>
            </CardElement>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import type { Battery, Total, Vedirect, Huawei, PowerMeter } from '@/types/LiveDataStatus';
import CardElement from './CardElement.vue';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
  props: {
    totalData: { type: Object as PropType<Total>, required: true },
    totalVeData: { type: Object as PropType<Vedirect>, required: true },
    totalBattData: { type: Object as PropType<Battery>, required: true },
    powerMeterData: { type: Object as PropType<Huawei>, required: true },
    huaweiData: { type: Object as PropType<PowerMeter>, required: true },
  },
});
</script>
