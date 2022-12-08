<template>
    <BasePage :title="'Login'" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <div class="card">
            <div class="card-header text-bg-danger">System Login</div>
            <div class="card-body">

                <form @submit.prevent="handleSubmit">
                    <div class="form-group">
                        <label for="username">Username</label>
                        <input type="text" v-model="username" name="username" class="form-control"
                            :class="{ 'is-invalid': submitted && !username }" />
                        <div v-show="submitted && !username" class="invalid-feedback">Username is required</div>
                    </div>
                    <div class="form-group">
                        <label htmlFor="password">Password</label>
                        <input type="password" v-model="password" name="password" class="form-control"
                            :class="{ 'is-invalid': submitted && !password }" />
                        <div v-show="submitted && !password" class="invalid-feedback">Password is required</div>
                    </div>
                    <div class="form-group">
                        <button class="btn btn-primary" :disabled="dataLoading">Login</button>
                    </div>
                </form>
            </div>
        </div>
    </BasePage>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import router from '@/router';
import { login } from '@/utils';
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
    },
    data() {
        return {
            dataLoading: false,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            returnUrl: '',
            username: '',
            password: '',
            submitted: false,
        };
    },
    created() {
        // get return url from route parameters or default to '/'
        this.returnUrl = this.$route.query.returnUrl?.toString() || '/';
    },
    methods: {
        handleSubmit() {
            this.submitted = true;
            const { username, password } = this;

            // stop here if form is invalid
            if (!(username && password)) {
                return;
            }

            this.dataLoading = true;
            login(username, password)
                .then(
                    () => {
                        this.$emitter.emit("logged-in");
                        router.push(this.returnUrl);
                    },
                    error => {
                        this.$emitter.emit("logged-out");
                        this.alertMessage = error;
                        this.alertType = 'danger';
                        this.showAlert = true;
                        this.dataLoading = false;
                    }
                )
        }
    }
});
</script>