<template>
  <tr>
    <th>{{ name }}</th>
    <td>
      <div class="progress">
        <div
          class="progress-bar"
          role="progressbar"
          :style="{ width: this.getPercent() + '%' }"
          v-bind:aria-valuenow="this.getPercent()"
          aria-valuemin="0"
          aria-valuemax="100"
        >
          {{ this.getPercent() }}%
        </div>
      </div>
    </td>
    <td class="rightCell">
      {{ Math.round((total - used) / 1024) }}
      KByte
    </td>
    <td class="rightCell">{{ Math.round(used / 1024) }} KByte</td>
    <td class="rightCell">{{ Math.round(total / 1024) }} KByte</td>
  </tr>
</template>

<script>
export default {
  props: {
    name: String,
    total: Number,
    used: Number,
  },
  methods: {
    getPercent() {
      return Math.round((this.used / this.total) * 100);
    },
  },
};
</script>
