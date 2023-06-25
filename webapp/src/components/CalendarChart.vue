<template>
    <div class="card" v-if="dataLoaded">
        <GChart type="Calendar" :data="chartData" :options="chartOptions" :settings="{ packages: ['calendar'] }" />
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { GChart } from 'vue-google-charts';
import { authHeader, handleResponse } from '@/utils/authentication';
var data_cal: any;

export const options_cal = {
    height: 270,
    colorAxis: {
        minValue: 0,
        colors: ['#FFFFFF', '#0000FF']
    },
    calendar: {
        cellSize: 24,
    },
};
export default defineComponent({
    components: {
        GChart,
    },
    data() {
        return {
            chartData: data_cal,
            chartOptions: options_cal,
            dataLoaded: false,
        }
    },
    created() {
        this.getInitialData();
    },
    methods: {
        getInitialData() {
            this.dataLoaded = false;
            fetch("/api/databaseDay", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((energy) => {
                    if (energy) {
                        this.chartData = [[{ type: 'date', id: 'Date' }, { type: 'number', id: 'Energy' }]];
                        var d: Date;
                        var a: any;
                        energy.forEach((x: any[]) => {
                            d = new Date(x[0] + 2000, x[1] - 1, x[2], x[3])
                            a = [d, Math.round(x[4])]
                            this.chartData.push(a)
                        })
                        this.dataLoaded = true;
                    }
                })
        }
    }
});
</script>
