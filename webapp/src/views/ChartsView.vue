<template>
  <BasePage :title="$t('charts.PowerProductionCharts')" :isLoading="dataLoading" showReload @reload="loadData">
    <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
      {{ alertMessage }}
    </BootstrapAlert>

    <CardElement :text="$t('charts.TimeRange')" textVariant="text-bg-primary">
      <div class="row mb-3">
        <div class="col-md-4">
          <label for="resolutionSelect" class="form-label">{{ $t('charts.Resolution') }}</label>
          <select id="resolutionSelect" class="form-select" v-model="selectedResolution">
            <option v-for="res in resolutions" :key="res.value" :value="res.value">
              {{ $t(`charts.${res.label}`) }}
            </option>
          </select>
        </div>
        <div class="col-md-4">
          <label for="startDate" class="form-label">{{ $t('charts.StartDate') }}</label>
          <input type="datetime-local" class="form-control" id="startDate" v-model="startDate">
        </div>
        <div class="col-md-4">
          <label for="endDate" class="form-label">{{ $t('charts.EndDate') }}</label>
          <input type="datetime-local" class="form-control" id="endDate" v-model="endDate">
        </div>
      </div>
      <div class="d-grid gap-2 d-md-flex justify-content-md-end">
        <button type="button" class="btn btn-primary" @click="loadData">{{ $t('charts.LoadData') }}</button>
      </div>
    </CardElement>

    <CardElement :text="$t('charts.PowerProduction')" textVariant="text-bg-primary" add-space>
      <div v-if="noDataAvailable" class="alert alert-info">
        {{ $t('charts.NoDataAvailable') }}
      </div>
      <HistoryBarChart v-else :chartData="chartData" :options="chartOptions" />
    </CardElement>

    <CardElement :text="$t('charts.PeakPower')" textVariant="text-bg-primary" add-space>
      <div v-if="noDataAvailable" class="alert alert-info">
        {{ $t('charts.NoDataAvailable') }}
      </div>
      <HistoryBarChart v-else :chartData="peakChartData" :options="peakChartOptions" />
    </CardElement>
  </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import HistoryBarChart from '@/components/HistoryBarChart.vue';
import type { HistoricalData } from '@/types/HistoricalData';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
  components: {
    BasePage,
    BootstrapAlert,
    CardElement,
    HistoryBarChart
  },
  data() {
    return {
      dataLoading: false,
      historicalData: null as HistoricalData | null,
      selectedResolution: 1, // Default to hourly
      resolutions: [
        { value: 0, label: 'Resolution20Min' },
        { value: 1, label: 'ResolutionHourly' },
        { value: 2, label: 'ResolutionDaily' },
        { value: 3, label: 'ResolutionMonthly' }
      ],
      startDate: '',
      endDate: '',
      alertMessage: '',
      alertType: 'info',
      showAlert: false,
      noDataAvailable: false
    };
  },
  created() {
    // Initialize date range to last 7 days
    const end = new Date();
    const start = new Date();
    start.setDate(start.getDate() - 7);
    
    this.startDate = this.formatDateTimeLocal(start);
    this.endDate = this.formatDateTimeLocal(end);
    
    this.loadData();
  },
  computed: {
    chartData() {
      if (!this.historicalData) return { datasets: [] };
      
      const labels = this.historicalData.timestamps.map(ts => 
        this.formatTimestamp(ts, this.selectedResolution));
      
      return {
        labels,
        datasets: [
          {
            label: this.$t('charts.PowerProduction'),
            data: this.historicalData.yields,
            backgroundColor: 'rgba(75, 192, 192, 0.6)',
            borderColor: 'rgba(75, 192, 192, 1)',
            borderWidth: 1
          }
        ]
      };
    },
    chartOptions() {
      return {
        scales: {
          y: {
            beginAtZero: true,
            title: {
              display: true,
              text: 'Wh'
            }
          }
        },
        plugins: {
          tooltip: {
            callbacks: {
              label: (context: any) => {
                return `${context.dataset.label}: ${this.$n(context.raw, 'decimal')} Wh`;
              }
            }
          }
        }
      };
    },
    peakChartData() {
      if (!this.historicalData) return { datasets: [] };
      
      const labels = this.historicalData.timestamps.map(ts => 
        this.formatTimestamp(ts, this.selectedResolution));
      
      return {
        labels,
        datasets: [
          {
            label: this.$t('charts.PeakPower'),
            data: this.historicalData.peak_powers,
            backgroundColor: 'rgba(255, 159, 64, 0.6)',
            borderColor: 'rgba(255, 159, 64, 1)',
            borderWidth: 1
          }
        ]
      };
    },
    peakChartOptions() {
      return {
        scales: {
          y: {
            beginAtZero: true,
            title: {
              display: true,
              text: 'W'
            }
          }
        },
        plugins: {
          tooltip: {
            callbacks: {
              label: (context: any) => {
                return `${context.dataset.label}: ${this.$n(context.raw, 'decimal')} W`;
              }
            }
          }
        }
      };
    }
  },
  methods: {
    loadData() {
      this.dataLoading = true;
      this.showAlert = false;
      this.noDataAvailable = false;
      
      // Convert date strings to Unix timestamps
      const startTimestamp = Math.floor(new Date(this.startDate).getTime() / 1000);
      const endTimestamp = Math.floor(new Date(this.endDate).getTime() / 1000);
      
      fetch(`/api/history/data?resolution=${this.selectedResolution}&start=${startTimestamp}&end=${endTimestamp}`, {
        headers: authHeader()
      })
        .then(response => handleResponse(response, this.$emitter, this.$router))
        .then(data => {
          this.historicalData = data;
          this.noDataAvailable = data.timestamps.length === 0;
          this.dataLoading = false;
        })
        .catch(error => {
          this.alertMessage = error.message || this.$t('charts.DataLoadError');
          this.alertType = 'danger';
          this.showAlert = true;
          this.dataLoading = false;
        });
    },
    formatDateTimeLocal(date: Date): string {
      // Format date as YYYY-MM-DDThh:mm (format required by datetime-local input)
      const pad = (num: number) => num.toString().padStart(2, '0');
      return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}T${pad(date.getHours())}:${pad(date.getMinutes())}`;
    },
    formatTimestamp(timestamp: number, resolution: number): string {
      const date = new Date(timestamp * 1000);
      
      switch (resolution) {
        case 0: // 20 min
          return `${date.toLocaleDateString()} ${date.getHours()}:${String(date.getMinutes()).padStart(2, '0')}`;
        case 1: // Hourly
          return `${date.toLocaleDateString()} ${date.getHours()}:00`;
        case 2: // Daily
          return date.toLocaleDateString();
        case 3: // Monthly
          return date.toLocaleDateString(this.$i18n.locale, { month: 'long', year: 'numeric' });
        default:
          return date.toLocaleString();
      }
    }
  }
});
</script>
