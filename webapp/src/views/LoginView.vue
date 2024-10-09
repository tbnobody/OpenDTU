<template>
    <BasePage :title="$t('login.Login')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <CardElement :text="$t('login.SystemLogin')" textVariant="text-bg-danger">
            <form @submit.prevent="handleSubmit">
                <div class="form-group">
                    <label for="username">{{ $t('login.Username') }}</label>
                    <input
                        type="text"
                        v-model="username"
                        name="username"
                        class="form-control"
                        :class="{ 'is-invalid': submitted && !username }"
                        @keydown.space.prevent
                    />
                    <div v-show="submitted && !username" class="invalid-feedback">
                        {{ $t('login.UsernameRequired') }}
                    </div>
                </div>
                <div class="form-group">
                    <label htmlFor="password">{{ $t('login.Password') }}</label>
                    <input
                        type="password"
                        v-model="password"
                        name="password"
                        class="form-control"
                        :class="{ 'is-invalid': submitted && !password }"
                    />
                    <div v-show="submitted && !password" class="invalid-feedback">
                        {{ $t('login.PasswordRequired') }}
                    </div>
                </div>
                <div class="form-group">
                    <button class="btn btn-primary" :disabled="dataLoading">
                        {{ $t('login.LoginButton') }}
                    </button>
                </div>
            </form>
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import router from '@/router';
import { login } from '@/utils';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
    },
    data() {
        return {
            dataLoading: false,
            alertMessage: '',
            alertType: 'info',
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
            login(username, password).then(
                () => {
                    this.$emitter.emit('logged-in');
                    router.push(this.returnUrl);
                },
                (error) => {
                    this.$emitter.emit('logged-out');
                    this.alertMessage = error;
                    this.alertType = 'danger';
                    this.showAlert = true;
                    this.dataLoading = false;
                }
            );
        },
    },
});
</script>
