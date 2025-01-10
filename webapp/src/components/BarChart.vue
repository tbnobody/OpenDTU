<template>
    <div class="card row" v-if="dataLoaded">
        <GChart type="ColumnChart" :data="chartData" :options="chartOptions" />
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { GChart } from 'vue-google-charts';
import { authHeader, handleResponse } from '@/utils/authentication';
//import { DatetimeFormat } from 'vue-i18n';

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let data_col: any;

const options_col = {
    height: 300,
    chartArea: {
        top: 25,
        width: '85%',
        height: '80%'
    },
    bar: {
        groupWidth: '90%'
    },
    legend: {
        position: 'none'
    },
    hAxis: {
        format: 'HH',
        minorGridlines: {
            count: 0
        }
    },
    vAxis: {
        minValue: 0,
        format: '# Wh'
    }
};
export default defineComponent({
    components: {
        GChart,
    },
    data() {
        return {
            chartData: data_col,
            chartOptions: options_col,
            dataLoaded: false,
        }
    },
    created() {
        this.getInitialData();
        this.startautorefresh();
    },
    methods: {
        getInitialData() {
            this.dataLoaded = false;
            fetch("/api/databaseHour", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((energy) => {
                    if (energy) {
                        this.chartData = [[{ type: 'date', id: 'Time' }, { type: 'number', id: 'Energy' }]];
                        // eslint-disable-next-line @typescript-eslint/no-explicit-any
                        energy.forEach((x: any[]) => {
                            const d = new Date(x[0] + 2000, x[1] - 1, x[2], x[3]);
                            this.chartData.push([d, Math.round(x[4])])
                        });
                        this.dataLoaded = true;
                    }
                });

            // let date_formatter = new google.visualization.DateFormat({
            //     pattern: "dd.MM.YY HH:mm"
            // });
            // date_formatter.format(data, 0);
        },
        callEveryHour() {
            this.getInitialData();
            setInterval(this.getInitialData, 1000 * 60 * 60);   // refresh every hour
        },
        startautorefresh() {
            const nextDate = new Date();
            nextDate.setHours(nextDate.getHours() + 1);
            nextDate.setMinutes(0);
            nextDate.setSeconds(5);
            const difference: number = nextDate.valueOf() - Date.now();
            setTimeout(this.callEveryHour, difference);
        },
    }
});
</script>
