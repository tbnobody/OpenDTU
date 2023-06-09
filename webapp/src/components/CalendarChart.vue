<template>
    <div class="card" :class="{}">
        <div v-if="dataBase.valid_data">
            <div class="card-header">
            </div>
            <div>
                <GoogleChart />
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, type PropType, h } from 'vue';
import type { DatabaseStatus } from "@/types/DatabaseStatus";
import { GChart } from 'vue-google-charts';

var data_cal: any;
export const type_cal = 'Calendar';
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
    props: {
        dataBase: { type: Object as PropType<DatabaseStatus>, required: true },
    },
    setup() {
        return () =>
            h(GChart, {
                data: data_cal,
                options: options_cal,
                type: type_cal,
                settings: {
                    packages: ['calendar'],
                }
            });
    },
    beforeMount() {
        this.drawChart()
    },
    mount() {
        this.drawChart()
    },
    methods: {
        drawChart() {
            data_cal = [[{
                type: 'date',
                id: 'Date'
            },
            {
                type: 'number',
                id: 'Energy'
            }]]
            var d: Date;
            var a: any;
            const energy = this.dataBase.values;
            var old_energy = 0.0;
            var last_energy = 0.0;
            var old_day = -1;
            energy.forEach((x: any[]) => {
                if (old_day < 0) {
                    old_day = x[2];
                    old_energy = x[4];
                } else {
                    if (x[2] != old_day) {
                        data_cal.push(a)
                        old_day = x[2]
                        old_energy = last_energy
                    }
                }
                last_energy = x[4]
                d = new Date(x[0] + 2000, x[1] - 1, x[2], x[3])
                a = [d, Math.round((last_energy - old_energy) * 1000)]
            })
            data_cal.push(a)
        }
    }
});
</script>
