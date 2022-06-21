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
              <FsInfo name="Heap" :total="systemDataList.heap_total" :used="systemDataList.heap_used" />
              <FsInfo name="LittleFs" :total="systemDataList.littlefs_total" :used="systemDataList.littlefs_used" />
              <FsInfo name="Sketch" :total="systemDataList.sketch_total" :used="systemDataList.sketch_used" />
          </tbody>
        </table>
      </div>
    </div>
  </div>
</template>

<script>
import { defineComponent } from 'vue';
import FsInfo from "@/components/partials/FsInfo.vue";

export default defineComponent({
  components: {
    FsInfo,
  },
  data() {
    return {
      systemDataList: [],
    };
  },
  created() {
    this.getSystemInfo();
  },
  methods: {
    getSystemInfo() {
      fetch("/api/system/status")
        .then((response) => response.json())
        .then((data) => (this.systemDataList = data));
    },
  },
});
</script>
