<template>
    <div class="card row" v-if="dataLoaded">
        <CalendarHeatmap
            :values="values"
            :round="1"
            :end-date="endDate"
            :style="{ 'font-size': '10px' }"
            tooltip-unit="Wh"
        />
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { CalendarHeatmap } from 'vue3-calendar-heatmap';
import { authHeader, handleResponse } from '@/utils/authentication';
const data: Array<{ date: Date; count: number }> = [];

export default defineComponent({
    components: {
        CalendarHeatmap,
    },
    data() {
        return {
            values: data,
            endDate: new Date(),
            dataLoaded: false,
        };
    },
    created() {
        this.getInitialData();
        this.startautorefresh();
    },
    methods: {
        getInitialData() {
            this.dataLoaded = false;
            fetch('/api/databaseDay', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((energy) => {
                    if (energy) {
                        this.values = [];
                        let d: Date;
                        // eslint-disable-next-line @typescript-eslint/no-explicit-any
                        energy.forEach((x: any[]) => {
                            d = new Date(x[0] + 2000, x[1] - 1, x[2], x[3]);
                            this.values.push({ date: d, count: Math.round(x[4]) });
                        });
                        this.dataLoaded = true;
                    }
                });
        },
        callEveryHour() {
            this.getInitialData();
            setInterval(this.getInitialData, 1000 * 60 * 60); // refresh every hour
        },
        startautorefresh() {
            const nextDate = new Date();
            nextDate.setHours(nextDate.getHours() + 1);
            nextDate.setMinutes(0);
            nextDate.setSeconds(5);
            const difference: number = nextDate.valueOf() - Date.now();
            setTimeout(this.callEveryHour, difference);
        },
    },
});
</script>
