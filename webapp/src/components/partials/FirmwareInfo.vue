<template>
  <div class="card">
    <div class="card-header text-white bg-primary">
      Firmware Information
    </div>
    <div class="card-body">
      <div class="table-responsive">
        <table class="table table-hover table-condensed">
          <tbody>
            <tr>
              <th>Hostname</th>
              <td>{{ systemDataList.hostname }}</td>
            </tr>
            <tr>
              <th>SDK Version</th>
              <td>{{ systemDataList.sdkversion }}</td>
            </tr>
            <tr>
              <th>Firmware Version</th>
              <td>{{ systemDataList.firmware_version }}</td>
            </tr>
            <tr>
              <th>Reset Reason CPU 0</th>
              <td>{{ systemDataList.resetreason_0 }}</td>
            </tr>
            <tr>
              <th>Reset Reason CPU 1</th>
              <td>{{ systemDataList.resetreason_1 }}</td>
            </tr>
            <tr>
              <th>Config save count</th>
              <td>{{ systemDataList.cfgsavecount }}</td>
            </tr>
            <tr>
              <th>Uptime</th>
              <td>{{ timeInHours(systemDataList.uptime) }}</td>
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
      systemDataList: [],
    };
  },
  created() {
    this.getSystemInfo();
  },
  computed: {
    timeInHours() {
      return (value) => {
        let days = parseInt(Math.floor(value / 3600 / 24));
        let hours = parseInt(Math.floor((value - days * 3600 * 24) / 3600));
        let minutes = parseInt(Math.floor((value - days * 3600 * 24 - hours * 3600) / 60));
        let seconds = parseInt((value - days * 3600 * 24 - hours * 3600 + minutes * 60) % 60);

        let dHours = hours > 9 ? hours : "0" + hours;
        let dMins = minutes > 9 ? minutes : "0" + minutes;
        let dSecs = seconds > 9 ? seconds : "0" + seconds;

        return days + " days " + dHours + ":" + dMins + ":" + dSecs;
      };
    },
  },
  methods: {
    getSystemInfo() {
      fetch("/api/system/status")
        .then((response) => response.json())
        .then((data) => (this.systemDataList = data));
    },
  },
};
</script>
