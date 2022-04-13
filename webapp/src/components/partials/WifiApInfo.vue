<template>
  <div class="card">
    <div class="card-header text-white bg-primary">
      WiFi Information (Access Point)
    </div>
    <div class="card-body">
      <div class="table-responsive">
        <table id="wifi_sta" class="table table-hover table-condensed">
          <tbody id="wifi_sta-data">
            <tr>
              <th>Status</th>
              <td
                class="badge"
                :class="{
                  'bg-danger': !networkDataList.ap_status,
                  'bg-success': networkDataList.ap_status,
                }"
              >
                <span v-if="networkDataList.ap_status">enabled</span>
                <span v-else>disabled</span>
              </td>
            </tr>
            <tr>
              <th>SSID</th>
              <td>{{ networkDataList.ap_ssid }}</td>
            </tr>
            <tr>
              <th># Stations</th>
              <td>{{ networkDataList.ap_stationnum }}</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      networkDataList: [],
    };
  },
  created() {
    this.getNetworkInfo();
  },
  methods: {
    getNetworkInfo() {
      fetch("/api/network/status")
        .then((response) => response.json())
        .then((data) => (this.networkDataList = data));
    },
    getRSSIasQuality(rssi) {
      var quality = 0;

      if (rssi <= -100) {
        quality = 0;
      } else if (rssi >= -50) {
        quality = 100;
      } else {
        quality = 2 * (rssi + 100);
      }

      return quality;
    },
  },
};
</script>
