<template>
  <div class="col">
    <div class="card">
      <div class="card-header">Channel {{ channelNumber }}</div>
      <div class="card-body">
        <table class="table table-striped table-hover">
          <thead>
            <tr>
              <th scope="col">Property</th>
              <th scope="col">Value</th>
              <th scope="col">Unit</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(property, key) in channelData" :key="`prop-${key}`">
              <th scope="row">{{ key }}</th>
              <td style="text-align: right">{{ formatNumber(property.v) }}</td>
              <td>{{ property.u }}</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  props: {
    channelData: Object,
    channelNumber: Number,
  },
  methods: {
    formatNumber(num) {
      return parseFloat(num).toFixed(2);
    },
  },
};
</script>
