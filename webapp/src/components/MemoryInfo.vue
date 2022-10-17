<template>
    <div class="card">
        <div class="card-header text-white bg-primary">Memory Information</div>
        <div class="card-body">
            <div class="table-responsive">
                <table class="table table-hover table-condensed">
                    <thead>
                        <tr>
                            <th>Type</th>
                            <th>Usage</th>
                            <th class="rightCell">Free</th>
                            <th class="rightCell">Used</th>
                            <th class="rightCell">Size</th>
                        </tr>
                    </thead>
                    <tbody>
                        <FsInfo name="Heap" :total="heap_total" :used="heap_used" />
                        <FsInfo name="LittleFs" :total="littlefs_total"
                            :used="littlefs_used" />
                        <FsInfo name="Sketch" :total="sketch_total" :used="sketch_used" />
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import FsInfo from "@/components/FsInfo.vue";

export default defineComponent({
    components: {
        FsInfo,
    },
    props: {
        heap_total: { type: Number, required: true },
        heap_used: { type: Number, required: true },
        littlefs_total: { type: Number, required: true },
        littlefs_used: { type: Number, required: true },
        sketch_total: { type: Number, required: true },
        sketch_used: { type: Number, required: true },
    },
});
</script>
