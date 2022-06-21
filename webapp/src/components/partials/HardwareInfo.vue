<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            Hardware Information
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Chip Model</th>
                            <td>{{ systemDataList.chipmodel }}</td>
                        </tr>
                        <tr>
                            <th>Chip Revision</th>
                            <td>{{ systemDataList.chiprevision }}</td>
                        </tr>
                        <tr>
                            <th>Chip Cores</th>
                            <td>{{ systemDataList.chipcores }}</td>
                        </tr>
                        <tr>
                            <th>CPU Frequency</th>
                            <td>{{ systemDataList.cpufreq }} MHz</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script>
import { defineComponent } from 'vue';

export default defineComponent({
    data() {
        return {
            systemDataList: [],
        };
    },
    created() {
        this.getSystemInfo();
    },
    methods: {
        getSystemInfo() {
            fetch("/api/system/status")
                .then((response) => response.json())
                .then((data) => (this.systemDataList = data));
        },
    },
});
</script>
