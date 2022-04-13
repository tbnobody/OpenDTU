<template>
  <div class="card">
    <div class="card-header text-white bg-primary">
      WiFi Information (Station)
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
                  'bg-danger': !networkDataList.sta_status,
                  'bg-success': networkDataList.sta_status,
                }"
              >
                <span v-if="networkDataList.sta_status">enabled</span>
                <span v-else>disabled</span>
              </td>
            </tr>
            <tr>
              <th>SSID</th>
              <td>{{ networkDataList.sta_ssid }}</td>
            </tr>
            <tr>
              <th>Quality</th>
              <td>{{ this.getRSSIasQuality(networkDataList.sta_rssi) }} %</td>
            </tr>
            <tr>
              <th>RSSI</th>
              <td>{{ networkDataList.sta_rssi }}</td>
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
