<template>
  <div class="container" role="main">
    <div class="page-header">
      <h1>Network Info</h1>
    </div>
    <WifiStationInfo />
    <div class="mt-5"></div>
    <WifiApInfo />
    <div class="mt-5"></div>
    <InterfaceStationInfo />
    <div class="mt-5"></div>
    <InterfaceApInfo />
    <div class="mt-5"></div>
  </div>
</template>

<script>
import { defineComponent } from 'vue';
import WifiStationInfo from "./partials/WifiStationInfo.vue";
import WifiApInfo from "./partials/WifiApInfo.vue";
import InterfaceStationInfo from "./partials/InterfaceStationInfo.vue";
import InterfaceApInfo from "./partials/InterfaceApInfo.vue";

export default defineComponent({
  components: {
    WifiStationInfo,
    WifiApInfo,
    InterfaceStationInfo,
    InterfaceApInfo,
  },
});
</script>