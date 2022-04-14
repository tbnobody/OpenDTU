<template>
  <div class="container" role="main">
    <div class="page-header">
      <h1>System Info</h1>
    </div>
    <div class="bg-light p-5 rounded">
      <FirmwareInfo />
      <HardwareInfo />
      <MemoryInfo />
    </div>
  </div>
</template>

<script>
import HardwareInfo from "@/components/partials/HardwareInfo.vue";
import FirmwareInfo from "@/components/partials/FirmwareInfo.vue";
import MemoryInfo from "@/components/partials/MemoryInfo.vue";

export default {
  components: {
    HardwareInfo,
    FirmwareInfo,
    MemoryInfo,
  },
};
</script>
