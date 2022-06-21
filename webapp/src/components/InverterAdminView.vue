<template>
  <div class="container" role="main">
    <div class="page-header">
      <h1>Inverter Settings</h1>
    </div>

    <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
      {{ this.alertMessage }}
    </BootstrapAlert>

    <div class="card">
      <div class="card-header text-white bg-primary">Add a new Inverter</div>
      <div class="card-body">
        <form class="form-inline" v-on:submit.prevent="onSubmit">
          <div class="form-group">
            <label>Serial</label>
            <input
              v-model="inverterData.serial"
              type="number"
              class="form-control ml-sm-2 mr-sm-4 my-2"
              required
            />
          </div>
          <div class="form-group">
            <label>Name</label>
            <input
              v-model="inverterData.name"
              type="text"
              class="form-control ml-sm-2 mr-sm-4 my-2"
              maxlength="31"
              required
            />
          </div>
          <div class="ml-auto text-right">
            <button type="submit" class="btn btn-primary my-2">Add</button>
          </div>
        </form>
      </div>
    </div>

    <div class="card mt-5">
      <div class="card-header text-white bg-primary">Inverter List</div>
      <div class="card-body">
        <div class="table-responsive">
          <table class="table">
            <thead>
              <tr>
                <th scope="col">Serial</th>
                <th>Name</th>
                <th>Type</th>
                <th>Action</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="inverter in sortedInverters" v-bind:key="inverter.id">
                <template v-if="editId == inverter.id">
                  <td>
                    <input
                      v-model="editInverterData.serial"
                      type="number"
                      class="form-control"
                    />
                  </td>
                  <td>
                    <input
                      v-model="editInverterData.name"
                      type="text"
                      class="form-control"
                      maxlength="31"
                    />
                  </td>
                  <td>
                    {{ editInverterData.type }}
                  </td>
                  <td>
                    <a href="#" class="icon">
                      <BIconCheck v-on:click="onEditSubmit(inverter.id)" />
                    </a>
                    <a href="#" class="icon">
                      <BIconX v-on:click="onCancel" />
                    </a>
                  </td>
                </template>
                <template v-else>
                  <td>
                    {{ inverter.serial }}
                  </td>
                  <td>
                    {{ inverter.name }}
                  </td>
                  <td>
                    {{ inverter.type }}
                  </td>
                  <td>
                    <a href="#" class="icon">
                      <BIconTrash v-on:click="onDelete(inverter.id)" />
                    </a>
                    <a href="#" class="icon">
                      <BIconPencil v-on:click="onEdit(inverter)" />
                    </a>
                  </td>
                </template>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import BootstrapAlert from "@/components/partials/BootstrapAlert.vue";

export default {
  components: {
    BootstrapAlert,
  },
  data() {
    return {
      editId: "-1",
      inverterData: {
        id: "",
        serial: "",
        name: "",
      },
      editInverterData: {
        id: "",
        serial: "",
        name: "",
        type: "",
      },
      inverters: [],
      alertMessage: "",
      alertType: "info",
      showAlert: false,
    };
  },
  created() {
    this.getInverters();
  },
  computed: {
    sortedInverters() {
      return this.inverters.slice().sort((a, b) => {
        return a.serial - b.serial;
      });
    },
  },
  methods: {
    getInverters() {
      fetch("/api/inverter/list")
        .then((response) => response.json())
        .then((data) => (this.inverters = data.inverter));
    },
    onSubmit() {
      const formData = new FormData();
      formData.append("data", JSON.stringify(this.inverterData));

      fetch("/api/inverter/add", {
        method: "POST",
        body: formData,
      })
        .then(function (response) {
          if (response.status != 200) {
            throw response.status;
          } else {
            return response.json();
          }
        })
        .then(
          function (response) {
            this.alertMessage = response.message;
            this.alertType = response.type;
            this.showAlert = true;
          }.bind(this)
        )
        .then(this.getInverters());

      this.inverterData.serial = "";
      this.inverterData.name = "";
    },
    onDelete(id) {
      const formData = new FormData();
      formData.append("data", JSON.stringify({ id: id }));

      fetch("/api/inverter/del", {
        method: "POST",
        body: formData,
      })
        .then(function (response) {
          if (response.status != 200) {
            throw response.status;
          } else {
            return response.json();
          }
        })
        .then(
          function (response) {
            this.alertMessage = response.message;
            this.alertType = response.type;
            this.showAlert = true;
          }.bind(this)
        )
        .then(this.getInverters());
    },
    onEdit(inverter) {
      this.editId = inverter.id;
      this.editInverterData.serial = inverter.serial;
      this.editInverterData.name = inverter.name;
      this.editInverterData.type = inverter.type;
    },
    onCancel() {
      this.editId = "-1";
      this.editInverterData.serial = "";
      this.editInverterData.name = "";
    },
    onEditSubmit(id) {
      const formData = new FormData();
      this.editInverterData.id = id;
      formData.append("data", JSON.stringify(this.editInverterData));

      fetch("/api/inverter/edit", {
        method: "POST",
        body: formData,
      })
        .then(function (response) {
          if (response.status != 200) {
            throw response.status;
          } else {
            return response.json();
          }
        })
        .then(
          function (response) {
            this.alertMessage = response.message;
            this.alertType = response.type;
            this.showAlert = true;
          }.bind(this)
        )
        .then(this.getInverters());

      this.editId = "-1";
      this.editInverterData.serial = "";
      this.editInverterData.name = "";
      this.editInverterData.type = "";
    },
  },
};
</script>