<template>
  <div class="chart-container">
    <canvas ref="chart"></canvas>
  </div>
</template>

<script lang="ts">
import { defineComponent, onMounted, watch, ref, PropType } from 'vue';
import { Chart, registerables } from 'chart.js';
import type { ChartData, ChartOptions } from 'chart.js';

// Register Chart.js components
Chart.register(...registerables);

export default defineComponent({
  props: {
    chartData: {
      type: Object as PropType<ChartData>,
      required: true
    },
    options: {
      type: Object as PropType<ChartOptions>,
      default: () => ({})
    }
  },
  setup(props) {
    const chartRef = ref<HTMLCanvasElement | null>(null);
    let chart: Chart | null = null;

    onMounted(() => {
      createChart();
    });

    watch(() => props.chartData, () => {
      if (chart) {
        chart.data = props.chartData;
        chart.update();
      }
    }, { deep: true });

    const createChart = () => {
      if (chartRef.value) {
        // Destroy existing chart if it exists
        if (chart) {
          chart.destroy();
        }

        // Create new chart
        chart = new Chart(chartRef.value, {
          type: 'bar',
          data: props.chartData,
          options: {
            responsive: true,
            maintainAspectRatio: false,
            ...props.options
          }
        });
      }
    };

    return {
      chart: chartRef
    };
  }
});
</script>

<style scoped>
.chart-container {
  position: relative;
  height: 400px;
  width: 100%;
}
</style>
