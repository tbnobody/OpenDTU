<template>
    <li class="nav-item dropdown">
        <button
            class="btn btn-link nav-link py-2 px-0 px-lg-2 dropdown-toggle d-flex align-items-center"
            id="bd-theme"
            type="button"
            aria-expanded="false"
            data-bs-toggle="dropdown"
            data-bs-display="static"
            aria-label="Toggle theme (auto)"
        >
            <BIconCircleHalf class="bi my-1 theme-icon-active" />
        </button>
        <ul class="dropdown-menu dropdown-menu-end">
            <li>
                <button
                    type="button"
                    class="dropdown-item d-flex align-items-center"
                    data-bs-theme-value="light"
                    aria-pressed="false"
                >
                    <BIconSunFill class="bi me-2 opacity-50 theme-icon" />
                    {{ $t('localeswitcher.Light') }}
                </button>
            </li>
            <li>
                <button
                    type="button"
                    class="dropdown-item d-flex align-items-center"
                    data-bs-theme-value="dark"
                    aria-pressed="false"
                >
                    <BIconMoonStarsFill class="bi me-2 opacity-50 theme-icon" />
                    {{ $t('localeswitcher.Dark') }}
                </button>
            </li>
            <li>
                <button
                    type="button"
                    class="dropdown-item d-flex align-items-center active"
                    data-bs-theme-value="auto"
                    aria-pressed="true"
                >
                    <BIconCircleHalf class="bi me-2 opacity-50 theme-icon" />
                    {{ $t('localeswitcher.Auto') }}
                </button>
            </li>
        </ul>
    </li>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { BIconCircleHalf, BIconSunFill, BIconMoonStarsFill } from 'bootstrap-icons-vue';

export default defineComponent({
    name: 'ThemeSwitcher',
    components: {
        BIconCircleHalf,
        BIconSunFill,
        BIconMoonStarsFill,
    },
    data() {
        return {
            storedTheme: 'auto',
        };
    },
    methods: {
        getPreferredTheme() {
            if (this.storedTheme) {
                return this.storedTheme;
            }
            return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
        },
        setTheme(theme: string) {
            if (theme === 'auto' && window.matchMedia('(prefers-color-scheme: dark)').matches) {
                document.documentElement.setAttribute('data-bs-theme', 'dark');
            } else {
                document.documentElement.setAttribute('data-bs-theme', theme);
            }
        },
        showActiveTheme(theme: string) {
            const activeThemeIcon = document.querySelector('.theme-icon-active');
            const btnToActive = document.querySelector(`[data-bs-theme-value="${theme}"]`);
            const svgOfActiveBtn = btnToActive?.querySelector('.theme-icon');

            document.querySelectorAll('[data-bs-theme-value]').forEach((element) => {
                element.classList.remove('active');
            });

            btnToActive?.classList.add('active');

            if (svgOfActiveBtn) {
                activeThemeIcon?.replaceChildren('*', svgOfActiveBtn?.cloneNode(true));
            }
        },
    },
    mounted() {
        this.storedTheme = localStorage.getItem('theme') || 'auto';
        this.setTheme(this.getPreferredTheme());
        this.showActiveTheme(this.getPreferredTheme());

        window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
            if (this.storedTheme !== 'light' && this.storedTheme !== 'dark') {
                this.setTheme(this.getPreferredTheme());
            }
        });

        document.querySelectorAll('[data-bs-theme-value]').forEach((toggle) => {
            toggle.addEventListener('click', () => {
                const theme = toggle.getAttribute('data-bs-theme-value') || 'auto';
                localStorage.setItem('theme', theme);
                this.setTheme(theme);
                this.showActiveTheme(theme);
            });
        });
    },
});
</script>
