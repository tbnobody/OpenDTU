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
            <template v-for="event in eventLogList.events">
                <tr>
                    <td>{{ timeInHours(event.start_time) }}</td>
                    <td>{{ timeInHours(event.end_time) }}</td>
                    <td>{{ event.message_id }}</td>
                    <td>{{ event.message }}</td>
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
