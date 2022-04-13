<template>
  <div class="card">
    <div class="card-header text-white bg-primary">
      Network Interface (Access Point)
    </div>
    <div class="card-body">
      <div class="table-responsive">
        <table id="wifi_sta" class="table table-hover table-condensed">
          <tbody id="wifi_sta-data">
            <tr>
              <th>IP Address</th>
              <td>{{ networkDataList.ap_ip }}</td>
            </tr>
            <tr>
              <th>MAC Address</th>
              <td>{{ networkDataList.ap_mac }}</td>
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
