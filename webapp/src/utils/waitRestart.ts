import type { Router } from 'vue-router';

export function waitRestart(router: Router) {
    setTimeout(() => {
        router.push('/wait');
    }, 1000);
}
