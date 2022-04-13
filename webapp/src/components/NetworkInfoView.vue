<template>
  <div class="container" role="main">
    <div class="page-header">
      <h1>Network Info</h1>
    </div>
    <div class="bg-light p-5 rounded">
      <WifiStationInfo />
      <WifiApInfo />
      <InterfaceStationInfo />
      <InterfaceApInfo />
    </div>
  </div>
</template>

<script>
import WifiStationInfo from "./partials/WifiStationInfo.vue";
import WifiApInfo from "./partials/WifiApInfo.vue";
import InterfaceStationInfo from "./partials/InterfaceStationInfo.vue";
import InterfaceApInfo from "./partials/InterfaceApInfo.vue";

export default {
  components: {
    WifiStationInfo,
    WifiApInfo,
    InterfaceStationInfo,
    InterfaceApInfo,
  },
};
</script>