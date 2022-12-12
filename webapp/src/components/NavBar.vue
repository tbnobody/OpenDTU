<template>
    <nav class="navbar navbar-expand-md navbar-dark fixed-top bg-dark">
        <div class="container-fluid">
            <a class="navbar-brand" href="/"><span class="text-warning"><BIconSun width="30" height="30" class="d-inline-block align-text-top"/></span> OpenDTU</a>
            <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarNavAltMarkup"
                aria-controls="navbarNavAltMarkup" aria-expanded="false" aria-label="Toggle navigation">
                <span class="navbar-toggler-icon"></span>
            </button>
            <div class="collapse navbar-collapse" ref="navbarCollapse" id="navbarNavAltMarkup">
                <ul class="navbar-nav me-auto">
                    <li class="nav-item">
                        <router-link @click="onClick" class="nav-link" to="/">Live Data</router-link>
                    </li>
                    <li class="nav-item dropdown">
                        <a class="nav-link dropdown-toggle" href="#" id="navbarScrollingDropdown" role="button"
                            data-bs-toggle="dropdown" aria-expanded="false">
                            Settings
                        </a>
                        <ul class="dropdown-menu" aria-labelledby="navbarScrollingDropdown">
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/network">Network Settings</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/ntp">NTP Settings</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/mqtt">MqTT Settings</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/inverter">Inverter Settings
                                </router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/security">Security Settings
                                </router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/dtu">DTU Settings</router-link>
                            </li>
                            <li>
                                <hr class="dropdown-divider" />
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/config">Config Management</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/firmware/upgrade">Firmware Upgrade</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/maintenance/reboot">Device Reboot</router-link>
                            </li>
                        </ul>
                    </li>
                    <li class="nav-item dropdown">
                        <a class="nav-link dropdown-toggle" href="#" id="navbarScrollingDropdown" role="button"
                            data-bs-toggle="dropdown" aria-expanded="false">
                            Info
                        </a>
                        <ul class="dropdown-menu" aria-labelledby="navbarScrollingDropdown">
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/system">System</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/network">Network</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/ntp">NTP</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/mqtt">MqTT</router-link>
                            </li>
                        </ul>
                    </li>
                    <li class="nav-item">
                        <router-link @click="onClick" class="nav-link" to="/about">About</router-link>
                    </li>
                </ul>
                <form class="d-flex" role="search">
                    <button v-if="isLogged" class="btn btn-outline-danger" @click="signout">Logout</button>
                    <button v-if="!isLogged" class="btn btn-outline-success" @click="signin">Login</button>
                </form>
            </div>
        </div>
    </nav>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { logout, isLoggedIn } from '@/utils/authentication';
import { BIconSun } from 'bootstrap-icons-vue';

export default defineComponent({
    components: {
        BIconSun,
    },
    data() {
        return {
            isLogged: this.isLoggedIn(),
        }
    },
    created() {
        this.$emitter.on("logged-in", () => {
            this.isLogged = this.isLoggedIn();
        });
        this.$emitter.on("logged-out", () => {
            this.isLogged = this.isLoggedIn();
        });
    },
    methods: {
        isLoggedIn,
        logout,
        signin(e: Event) {
            e.preventDefault();
            this.$router.push('/login');
        },
        signout(e: Event) {
            e.preventDefault();
            this.logout();
            this.$emitter.emit("logged-out");
            this.$router.push('/');
        },
        onClick() {
            this.$refs.navbarCollapse && (this.$refs.navbarCollapse as HTMLElement).classList.remove("show");
        }
    },
});
</script>
