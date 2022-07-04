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
                            <td>{{ chipmodel }}</td>
                        </tr>
                        <tr>
                            <th>Chip Revision</th>
                            <td>{{ chiprevision }}</td>
                        </tr>
                        <tr>
                            <th>Chip Cores</th>
                            <td>{{ chipcores }}</td>
                        </tr>
                        <tr>
                            <th>CPU Frequency</th>
                            <td>{{ cpufreq }} MHz</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    props: {
        chipmodel: String,
        chiprevision: String,
        chipcores: String,
        cpufreq: String,
    },
});
</script>
