<template>
    <nav class="navbar navbar-expand-md navbar-dark fixed-top bg-dark">
        <div class="container-fluid">
            <router-link @click="onClick" class="navbar-brand" to="/"><span class="text-warning"><BIconSun width="30" height="30" class="d-inline-block align-text-top"/></span> OpenDTU</router-link>
            <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarNavAltMarkup"
                aria-controls="navbarNavAltMarkup" aria-expanded="false" aria-label="Toggle navigation">
                <span class="navbar-toggler-icon"></span>
            </button>
            <div class="collapse navbar-collapse" ref="navbarCollapse" id="navbarNavAltMarkup">
                <ul class="navbar-nav me-auto">
                    <li class="nav-item">
                        <router-link @click="onClick" class="nav-link" to="/">{{ $t('menu.LiveView') }}</router-link>
                    </li>
                    <li class="nav-item dropdown">
                        <a class="nav-link dropdown-toggle" href="#" id="navbarScrollingDropdown" role="button"
                            data-bs-toggle="dropdown" aria-expanded="false">
                            {{ $t('menu.Settings') }}
                        </a>
                        <ul class="dropdown-menu" aria-labelledby="navbarScrollingDropdown">
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/network">{{ $t('menu.NetworkSettings') }}</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/ntp">{{ $t('menu.NTPSettings') }}</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/mqtt">{{ $t('menu.MQTTSettings') }}</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/inverter">{{ $t('menu.InverterSettings') }}
                                </router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/security">{{ $t('menu.SecuritySettings') }}
                                </router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/dtu">{{ $t('menu.DTUSettings') }}</router-link>
                            </li>
                            <li>
                                <hr class="dropdown-divider" />
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/settings/config">{{ $t('menu.ConfigManagement') }}</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/firmware/upgrade">{{ $t('menu.FirmwareUpgrade') }}</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/maintenance/reboot">{{ $t('menu.DeviceReboot') }}</router-link>
                            </li>
                        </ul>
                    </li>
                    <li class="nav-item dropdown">
                        <a class="nav-link dropdown-toggle" href="#" id="navbarScrollingDropdown" role="button"
                            data-bs-toggle="dropdown" aria-expanded="false">
                            {{ $t('menu.Info') }}
                        </a>
                        <ul class="dropdown-menu" aria-labelledby="navbarScrollingDropdown">
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/system">{{ $t('menu.System') }}</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/network">{{ $t('menu.Network') }}</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/ntp">{{ $t('menu.NTP') }}</router-link>
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/mqtt">{{ $t('menu.MQTT') }}</router-link>
                            </li>
                            <li>
                                <hr class="dropdown-divider" />
                            </li>
                            <li>
                                <router-link @click="onClick" class="dropdown-item" to="/info/console">{{ $t('menu.Console') }}</router-link>
                            </li>
                        </ul>
                    </li>
                    <li class="nav-item">
                        <router-link @click="onClick" class="nav-link" to="/about">{{ $t('menu.About') }}</router-link>
                    </li>
                </ul>
                <form class="d-flex" role="search">
                    <button v-if="isLogged" class="btn btn-outline-danger" @click="signout">{{ $t('menu.Logout') }}</button>
                    <button v-if="!isLogged" class="btn btn-outline-success" @click="signin">{{ $t('menu.Login') }}</button>
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
