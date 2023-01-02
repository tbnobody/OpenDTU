<template>
    <tr>
        <th>{{ name }}</th>
        <td>
            <div class="progress">
                <div class="progress-bar" role="progressbar" :style="{ width: getPercent() + '%' }"
                    v-bind:aria-valuenow="getPercent()" aria-valuemin="0" aria-valuemax="100">
                    {{ $n(getPercent() / 100, 'percent') }}
                </div>
            </div>
        </td>
        <td class="rightCell">
            {{ $n(Math.round((total - used) / 1024), 'kilobyte') }}
        </td>
        <td class="rightCell">{{ $n(Math.round(used / 1024), 'kilobyte') }}</td>
        <td class="rightCell">{{ $n(Math.round(total / 1024), 'kilobyte') }}</td>
    </tr>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

export default defineComponent({
    props: {
        name: String,
        total: { type: Number, required: true },
        used: { type: Number, required: true },
    },
    methods: {
        getPercent() {
            return this.total === 0 ? 0 : Math.round((this.used / this.total) * 100);
        },
    },
});
</script>
