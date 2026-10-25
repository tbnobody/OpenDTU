<template>
    <dl class="row mb-3">
        <dt class="col-sm-8">{{ $t('eventlog.ReportedCount') }}</dt>
        <dd class="col-sm-4">{{ eventLogList.reported_count }}</dd>
        <dt class="col-sm-8">{{ $t('eventlog.StoredCount') }}</dt>
        <dd class="col-sm-4">{{ eventLogList.count }}</dd>
    </dl>

    <p class="text-body-secondary">{{ $t('eventlog.ReportedCountHint') }}</p>

    <div v-if="eventLogList.last_request_status === 'failure'" class="alert alert-warning" role="alert">
        {{ $t('eventlog.DetailsUnavailable') }}
    </div>
    <div v-else-if="eventLogList.last_request_status === 'pending'" class="alert alert-info" role="status">
        {{ $t('eventlog.DetailsPending') }}
    </div>

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
            <template v-for="(event, index) in eventLogList.events" :key="index">
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
