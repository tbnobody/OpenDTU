<template>
    <BasePage :title="$t('wait.NotReady')">
        <CardElement :text="$t('wait.PleaseWait')" textVariant="text-bg-primary">
            <div class="text-center">
                <div class="spinner-border" role="status">
                    <span class="visually-hidden"></span>
                </div>
            </div>
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import CardElement from '@/components/CardElement.vue';
import { authHeader } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        CardElement,
    },
    data() {
        return {
            hostCheckInterval: 0,
        };
    },
    mounted() {
        this.hostCheckInterval = setInterval(this.checkRemoteHostAndReload, 1000);
    },
    unmounted() {
        clearInterval(this.hostCheckInterval);
    },
    methods: {
        checkRemoteHostAndReload(): void {
            // Check if the browser is online
            if (navigator.onLine) {
                const remoteHostUrl = '/api/system/status';

                // Use a simple fetch request to check if the remote host is reachable
                fetch(remoteHostUrl, { headers: authHeader() })
                    .then((response) => {
                        // Check if the response status is OK (200-299 range)
                        if (response.ok) {
                            console.log('Remote host is available. Reloading page...');
                            clearInterval(this.hostCheckInterval);
                            this.hostCheckInterval = 0;
                            // Perform a page reload
                            window.location.replace('/');
                        } else {
                            console.log('Remote host is not reachable. Do something else if needed.');
                        }
                    })
                    .catch((error) => {
                        console.error('Error checking remote host:', error);
                    });
            } else {
                console.log('Browser is offline. Cannot check remote host.');
            }
        },
    },
});
</script>
