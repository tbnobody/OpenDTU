<template>
    <table class="table table-hover">
        <thead>
            <tr>
                <th scope="col">{{ $t('eventlog.Start') }}</th>
                <th scope="col">{{ $t('eventlog.Stop') }}</th>
                <th scope="col">{{ $t('eventlog.Id') }}</th>
                <th scope="col">{{ $t('eventlog.Message') }}</th>
            </tr>
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
import type { EventlogItems } from '@/types/EventlogStatus';
import { timestampToString } from '@/utils';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    props: {
        eventLogList: { type: Object as PropType<EventlogItems>, required: true },
    },
    computed: {
        timeInHours() {
            return (value: number) => {
                return timestampToString(this.$i18n.locale, value)[0];
            };
        },
    },
});
</script>
