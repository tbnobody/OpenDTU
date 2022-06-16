<template>
  <div class="container" role="main">
    <div class="page-header">
      <h1>Live Data</h1>
      <template v-if="waitForData == true">Waiting for data... </template>
      <template v-else>
        <div class="d-flex align-items-start">
          <div
            class="nav flex-column nav-pills me-3"
            id="v-pills-tab"
            role="tablist"
            aria-orientation="vertical"
          >
            <button
              v-for="inverter in inverterData"
              :key="inverter.serial"
              class="nav-link"
              :id="'v-pills-' + inverter.serial + '-tab'"
              data-bs-toggle="pill"
              :data-bs-target="'#v-pills-' + inverter.serial"
              type="button"
              role="tab"
              aria-controls="'v-pills-' + inverter.serial"
              aria-selected="true"
            >
              {{ inverter.name }}
            </button>
          </div>

          <div class="tab-content" id="v-pills-tabContent">
            <div
              v-for="inverter in inverterData"
              :key="inverter.serial"
              class="tab-pane fade show"
              :id="'v-pills-' + inverter.serial"
              role="tabpanel"
              :aria-labelledby="'v-pills-' + inverter.serial + '-tab'"
              tabindex="0"
            >
              <div class="card">
                <div class="card-header text-white bg-primary">
                  {{ inverter.name }} ({{ inverter.serial }})
                </div>
                <div class="card-body">
                  <div class="row row-cols-1 row-cols-md-3 g-4">
                    <div v-for="channel in 5" :key="channel">
                      <InverterChannelInfo
                        v-if="inverter[channel - 1]"
                        :channelData="inverter[channel - 1]"
                        :channelNumber="channel - 1"
                      />
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </template>
    </div>
  </div>
</template>

<script>
import InverterChannelInfo from "@/components/partials/InverterChannelInfo";

export default {
  components: {
    InverterChannelInfo,
  },
  data() {
    return {
      connection: null,
      waitForData: true,
      inverterData: [],
    };
  },
  created() {
    console.log("Starting connection to WebSocket Server");

    const socketProtocol =
      window.location.protocol === "https:" ? "wss:" : "ws:";
    const port = window.location.port;
    const host = window.location.hostname;
    const webSocketUrl = socketProtocol + "//" + host + ":" + port + "/ws";

    this.connection = new WebSocket(webSocketUrl);

    this.connection.onmessage = function (event) {
      console.log(event);
      this.inverterData = JSON.parse(event.data);
      this.waitForData = false;
    }.bind(this);

    this.connection.onopen = function (event) {
      console.log(event);
      console.log("Successfully connected to the echo websocket server...");
    };
  },
};
</script>