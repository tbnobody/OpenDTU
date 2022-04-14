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
  methods: {
    getSystemInfo() {
      fetch("/api/system/status")
        .then((response) => response.json())
        .then((data) => (this.systemDataList = data));
    },
  },
};
</script>
