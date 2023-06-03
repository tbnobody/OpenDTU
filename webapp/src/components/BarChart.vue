<template>
    <div class="card" :class="{}">
        <div v-if="dataBase.valid_data">
            <div class="card-header">
                {{ $t('chart.LastDay') }}
            </div>
            <GoogleChart />
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, type PropType, h } from 'vue';
import type { DatabaseStatus } from "@/types/DatabaseStatus";
import { GChart } from 'vue-google-charts';
//import { DatetimeFormat } from 'vue-i18n';

var data: any;
export const type = 'ColumnChart';
export const options = {
    height: 600,
    chartArea: { top: 25, width: '85%', height: '80%' },
    legend: {
        position: 'none'
    },
    hAxis: {
        format: 'dd.MM.YY HH:mm',
        slantedText: true
    },
    vAxis: {
        format: '# Wh'
    }
};
export default defineComponent({
    components: {
        GChart,
    },
    props: {
        dataBase: { type: Object as PropType<DatabaseStatus>, required: true },
    },
    setup() {
        return () =>
            h(GChart, {
                data,
                options,
                type
            });
    },
    beforeMount() {
        this.drawChart()
    },
    methods: {
        drawChart() {
            var end = new Date()
            var start = new Date()
            var interval = 1    // number of days to show in the chart
            start.setDate(end.getDate() - interval)
            start.setHours(start.getHours() - 2)
            const energy = this.dataBase.values
            var old_energy = energy[0][4]
            data = [['Time', 'Energy']];
            energy.forEach((x: any[]) => {
                var d = new Date(x[0] + 2000, x[1] - 1, x[2], x[3])
                if ((d >= start) && (d <= end)) {
                    var a = [d, (x[4] - old_energy) * 1000]
                    data.push(a)
                }
                old_energy = x[4]
            })

            // var date_formatter = new google.visualization.DateFormat({
            //     pattern: "dd.MM.YY HH:mm"
            // });
            // date_formatter.format(data, 0);
        }
    }
});
</script>
