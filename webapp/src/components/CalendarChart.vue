<template>
    <div class="card" :class="{}">
        <div v-if="dataBase.valid_data">
            <div class="card-header">
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
export const type = 'Calendar';
export const options = {
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
                type,
                settings: {
                    packages: ['calendar']
                }
            });
    },
    beforeMount() {
        this.drawChart()
    },
    methods: {
        drawChart() {
            data = [[{
                type: 'date',
                id: 'Date'
            },
            {
                type: 'number',
                id: 'Energy'
            }]]
            var d: Date
            var a: any
            const energy = this.dataBase.values
            var old_energy = energy[0][4]
            var old_day = energy[0][2]
            var last_energy: number
            energy.forEach((x: any[]) => {
                if (x[2] != old_day) {
                    data.push(a)
                    old_day = x[2]
                    old_energy = last_energy
                }
                last_energy = x[4]
                d = new Date(x[0] + 2000, x[1] - 1, x[2], x[3])
                a = [d, ((last_energy - old_energy) * 1000).toFixed(0)]
            })
            data.push(a)

            // var date_formatter = new google.visualization.DateFormat({
            //     pattern: "dd.MM.YY HH:mm"
            // });
            // date_formatter.format(data, 0);
        }
    }
});
</script>
