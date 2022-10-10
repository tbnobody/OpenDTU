<template>
    <div class="card">
        <div class="card-header text-white bg-primary">
            Radio Information
        </div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <tbody>
                        <tr>
                            <th>Chip Status</th>
                            <td class="badge" :class="{
                                'bg-danger': !radio_connected,
                                'bg-success': radio_connected,
                            }">
                                <span v-if="radio_connected">connected</span>
                                <span v-else>not connected</span>
                            </td>
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
        radio_connected: { type: Boolean, required: true },
    },
});
</script>
