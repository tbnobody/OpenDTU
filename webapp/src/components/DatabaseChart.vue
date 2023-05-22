<template>
    <div class="card" :class="{}">
        <div class="card-header">
            {{ $t('databasechart.LastDay') }}
        </div>

        <div class="card-body">
            <div id="chart_div"></div>
        </div>
    </div>
</template>

<script lang="ts">
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    mounted() {
        this.drawChart()
    },
    methods: {
        drawChart() {
            google.charts.load('current', { packages: ['corechart', 'bar'] });
            fetch("/api/database", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((energy) => {
                    function drawBasic() {
                        var data = new google.visualization.DataTable();
                        data.addColumn('datetime', 'Time');
                        data.addColumn('number', 'Energy');
                        var old_energy = energy[0][4]
                        energy.forEach((x: any[]) => {
                            var d = new Date(x[0] + 2000, x[1] - 1, x[2], x[3])
                            if ((d >= start) && (d <= end)) {
                                data.addRow([d, (x[4] - old_energy) * 1000])
                            }
                            old_energy = x[4]
                        })

                        var date_formatter = new google.visualization.DateFormat({
                            pattern: "dd.MM.YY HH:mm"
                        });
                        date_formatter.format(data, 0);

                        var options = {
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
                        }

                        var chart = new google.visualization.ColumnChart(document.getElementById('chart_div'));
                        chart.draw(data, options);
                    }

                    var end = new Date()
                    var start = new Date()
                    var interval = 1    // number of days to show in the chart
                    start.setDate(end.getDate() - interval)
                    start.setHours(start.getHours() - 2)
                    google.charts.setOnLoadCallback(drawBasic());
                });
        }
    }
});
</script>
