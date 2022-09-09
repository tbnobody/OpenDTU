<template>
    <table class="table table-hover">
        <tbody>
            <tr>
                <td>Current Limit</td>
                <td>{{ formatNumber(limitData.limit) }}%</td>
            </tr>
        </tbody>
    </table>
</template>

<script lang="ts">
import { defineComponent } from 'vue';

declare interface LimitData {
    limit: number,
}

export default defineComponent({
    props: {
        limitData: { type: Object as () => LimitData, required: true },
    },
    computed: {
        formatNumber() {
            return (num: number) => {
                return new Intl.NumberFormat(
                    undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 }
                ).format(num)
            };
        },
    },
});
</script>