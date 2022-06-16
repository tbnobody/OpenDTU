<template>
  <div class="container" role="main">
    <div class="page-header">
      <h1>MqTT Info</h1>
    </div>

    <div class="card">
      <div class="card-header text-white bg-primary">Configuration Summary</div>
      <div class="card-body">
        <div class="table-responsive">
          <table class="table table-hover table-condensed">
            <tbody>
              <tr>
                <th>Status</th>
                <td
                  class="badge"
                  :class="{
                    'bg-danger': !mqttDataList.mqtt_enabled,
                    'bg-success': mqttDataList.mqtt_enabled,
                  }"
                >
                  <span v-if="mqttDataList.mqtt_enabled">enabled</span>
                  <span v-else>disabled</span>
                </td>
              </tr>
              <tr>
                <th>Server</th>
                <td>{{ mqttDataList.mqtt_hostname }}</td>
              </tr>
              <tr>
                <th>Port</th>
                <td>{{ mqttDataList.mqtt_port }}</td>
              </tr>
              <tr>
                <th>Username</th>
                <td>{{ mqttDataList.mqtt_username }}</td>
              </tr>
              <tr>
                <th>Base Topic</th>
                <td>{{ mqttDataList.mqtt_topic }}</td>
              </tr>
              <tr>
                <th>Publish Tnterval</th>
                <td>{{ mqttDataList.mqtt_publish_interval }} seconds</td>
              </tr>
              <tr>
                <th>Retain</th>
                <td
                  class="badge"
                  :class="{
                    'bg-danger': !mqttDataList.mqtt_retain,
                    'bg-success': mqttDataList.mqtt_retain,
                  }"
                >
                  <span v-if="mqttDataList.mqtt_retain">enabled</span>
                  <span v-else>disabled</span>
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <div class="card mt-5">
      <div class="card-header text-white bg-primary">Runtime Summary</div>
      <div class="card-body">
        <div class="table-responsive">
          <table class="table table-hover table-condensed">
            <tbody>
              <tr>
                <th>Connection Status</th>
                <td
                  class="badge"
                  :class="{
                    'bg-danger': !mqttDataList.mqtt_connected,
                    'bg-success': mqttDataList.mqtt_connected,
                  }"
                >
                  <span v-if="mqttDataList.mqtt_connected">connected</span>
                  <span v-else>disconnected</span>
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      mqttDataList: [],
    };
  },
  created() {
    this.getNtpInfo();
  },
  methods: {
    getNtpInfo() {
      fetch("/api/mqtt/status")
        .then((response) => response.json())
        .then((data) => (this.mqttDataList = data));
    },
  },
};
</script>