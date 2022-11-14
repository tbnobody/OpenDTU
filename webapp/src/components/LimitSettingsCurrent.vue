<template>
    <table class="table table-hover">
        <tbody>
            <tr>
                <td>Current Limit</td>
                <td>{{ formatNumber(limitData.limit, 2) }}%</td>
            </tr>
        </tbody>
    </table>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { formatNumber } from '@/utils';

declare interface LimitData {
    limit: number;
}

export default defineComponent({
    props: {
        limitData: { type: Object as () => LimitData, required: true },
    },
    methods: {
        formatNumber,
    }
});
</script>