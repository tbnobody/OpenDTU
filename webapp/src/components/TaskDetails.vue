<template>
    <CardElement :text="$t('taskdetails.TaskDetails')" textVariant="text-bg-primary" table>
        <div class="table-responsive">
            <table class="table table-hover table-condensed">
                <tbody>
                    <tr>
                        <th>{{ $t('taskdetails.Name') }}</th>
                        <th>{{ $t('taskdetails.StackFree') }}</th>
                        <th>{{ $t('taskdetails.Priority') }}</th>
                    </tr>
                    <tr v-for="task in taskDetails" v-bind:key="task.name">
                        <td>{{ $te(taskLangToken(task.name)) ? $t(taskLangToken(task.name)) : task.name }}</td>
                        <td>{{ $n(task.stack_watermark, 'byte') }}</td>
                        <td>{{ task.priority }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </CardElement>
</template>

<script lang="ts">
import CardElement from '@/components/CardElement.vue';
import type { TaskDetail } from '@/types/SystemStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        taskDetails: { type: Array as PropType<TaskDetail[]>, required: true },
    },
    methods: {
        taskLangToken(rawTask: string) {
            return 'taskdetails.Task_' + rawTask.replace(/[^A-Za-z0-9]/g, '').toLowerCase();
        },
    },
});
</script>
