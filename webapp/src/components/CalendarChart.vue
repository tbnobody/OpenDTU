<template>
    <div class="card" :class="{}">
        <div class="card-header">
        </div>
        <GoogleChart />
    </div>
</template>

<script lang="ts">
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent, h } from 'vue';
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
    mounted() {
        this.drawChart()
    },
    methods: {
        drawChart() {
            fetch("/api/database", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((energy) => {
                    data = [[{
                        type: 'date',
                        id: 'Date'
                    },
                    {
                        type: 'number',
                        id: 'Energy'
                    }]]
                    var d : Date
                    var a : any
                    var old_energy = energy[0][4]
                    var old_day = energy[0][2]
                    var last_energy : number
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
                });
        }
    }
});
</script>
