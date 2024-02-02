<template>
    <nav class="navbar navbar-expand-md fixed-top bg-body-tertiary" data-bs-theme="dark">
        <div class="container-fluid">
            <router-link @click="onClick" class="navbar-brand" to="/" style="display: flex; height: 30px; padding: 0;">
                <BIconTree v-if="isXmas" width="30" height="30" class="d-inline-block align-text-top text-success" />

                <BIconEgg v-else-if="isEaster" width="30" height="30" class="d-inline-block align-text-top text-info" />

                <BIconSun v-else width="30" height="30" class="d-inline-block align-text-top text-warning" />

                <span style="margin-left: .5rem">
                    OpenDTU
                </span>
            </router-link>
            <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarNavAltMarkup"
                aria-controls="navbarNavAltMarkup" aria-expanded="false" aria-label="Toggle navigation">
                <span class="navbar-toggler-icon"></span>
            </button>
            <div class="collapse navbar-collapse" ref="navbarCollapse" id="navbarNavAltMarkup">
                <ul class="navbar-nav navbar-nav-scroll d-flex me-auto flex-sm-fill">
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
                                <router-link @click="onClick" class="dropdown-item" to="/settings/device">{{ $t('menu.DeviceManager') }}</router-link>
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
                    <li class="flex-sm-fill"></li>
                    <ThemeSwitcher class="me-2" />
                    <form class="d-flex" role="search">
                        <LocaleSwitcher class="me-2" />
                        <button v-if="isLogged" class="btn btn-outline-danger" @click="signout">{{ $t('menu.Logout') }}</button>
                        <button v-if="!isLogged" class="btn btn-outline-success" @click="signin">{{ $t('menu.Login') }}</button>
                    </form>
                </ul>
            </div>
        </div>
    </nav>
</template>

<script lang="ts">
import { isLoggedIn, logout } from '@/utils/authentication';
import { BIconEgg, BIconSun, BIconTree } from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';
import LocaleSwitcher from './LocaleSwitcher.vue';
import ThemeSwitcher from './ThemeSwitcher.vue';

export default defineComponent({
    components: {
        BIconEgg,
        BIconSun,
        BIconTree,
        LocaleSwitcher,
        ThemeSwitcher,
    },
    data() {
        return {
            isLogged: this.isLoggedIn(),
            now: {} as Date,
        }
    },
    created() {
        this.$emitter.on("logged-in", () => {
            this.isLogged = this.isLoggedIn();
        });
        this.$emitter.on("logged-out", () => {
            this.isLogged = this.isLoggedIn();
        });

        this.now = new Date();
        setInterval(() => {
            this.now = new Date();
        }, 10000)
    },
    computed: {
        isXmas() {
            return (this.now.getMonth() + 1 == 12 && (this.now.getDate() >= 24 && this.now.getDate() <= 26));
        },
        isEaster() {
            const easter = this.getEasterSunday(this.now.getFullYear());
            var easterStart = new Date(easter);
            var easterEnd = new Date(easter);
            easterStart.setDate(easterStart.getDate() - 2);
            easterEnd.setDate(easterEnd.getDate() + 1);
            return this.now >= easterStart && this.now < easterEnd;
        },
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
        },
        getEasterSunday(year: number): Date {
            var f = Math.floor;
            var G = year % 19;
            var C = f(year / 100);
            var H = (C - f(C / 4) - f((8 * C + 13) / 25) + 19 * G + 15) % 30;
            var I = H - f(H / 28) * (1 - f(29 / (H + 1)) * f((21 - G) / 11));
            var J = (year + f(year / 4) + I + 2 - C + f(C / 4)) % 7;
            var L = I - J;
            var month = 3 + f((L + 40) / 44);
            var day = L + 28 - 31 * f(month / 4);

            return new Date(year, month - 1, day);
        }
    },
});
</script>