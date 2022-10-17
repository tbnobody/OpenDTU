<template>
    <table class="table table-hover">
        <thead>
            <th scope="col">Start</th>
            <th scope="col">Stop</th>
            <th scope="col">ID</th>
            <th scope="col">Message</th>
        </thead>
        <tbody>
            <template v-for="event in eventLogList.count" :key="event">
                <tr>
                    <td>{{ timeInHours(eventLogList.events[event - 1].start_time) }}</td>
                    <td>{{ timeInHours(eventLogList.events[event - 1].end_time) }}</td>
                    <td>{{ eventLogList.events[event - 1].message_id }}</td>
                    <td>{{ eventLogList.events[event - 1].message }}</td>
                </tr>
            </template>
        </tbody>
    </table>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { timestampToString } from '@/utils';

declare interface EventData {
    message_id: number,
    message: string,
    start_time: number,
    end_time: number
}

declare interface EventLogData {
    count: number,
    events: { [key: number]: EventData }
}

export default defineComponent({
    props: {
        eventLogList: { type: Object as () => EventLogData, required: true },
    },
    computed: {
        timeInHours() {
            return (value: number) => {
                return timestampToString(value);
            };
        },
    },
});
</script>