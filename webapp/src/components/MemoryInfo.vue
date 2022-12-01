<template>
    <div class="card">
        <div class="card-header text-bg-primary">{{ $t('memoryinfo.MemoryInformation') }}</div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <thead>
                        <tr>
                            <th>{{ $t('memoryinfo.Type') }}</th>
                            <th>{{ $t('memoryinfo.Usage') }}</th>
                            <th class="rightCell">{{ $t('memoryinfo.Free') }}</th>
                            <th class="rightCell">{{ $t('memoryinfo.Used') }}</th>
                            <th class="rightCell">{{ $t('memoryinfo.Size') }}</th>
                        </tr>
                    </thead>
                    <tbody>
                        <FsInfo :name="$t('memoryinfo.Heap')" :total="systemStatus.heap_total"
                            :used="systemStatus.heap_used" />
                        <FsInfo :name="$t('memoryinfo.LittleFs')" :total="systemStatus.littlefs_total"
                            :used="systemStatus.littlefs_used" />
                        <FsInfo :name="$t('memoryinfo.Sketch')" :total="systemStatus.sketch_total"
                            :used="systemStatus.sketch_used" />
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, type PropType } from 'vue';
import type { SystemStatus } from '@/types/SystemStatus';
import FsInfo from "@/components/FsInfo.vue";

export default defineComponent({
    components: {
        FsInfo,
    },
    props: {
        systemStatus: { type: Object as PropType<SystemStatus>, required: true },
    },
});
</script>
