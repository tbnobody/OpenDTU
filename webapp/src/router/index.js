import { createWebHistory, createRouter } from 'vue-router';
import HomeView from '@/components/HomeView'
import AboutView from '@/components/AboutView'

const routes = [
    {
        path: '/',
        name: 'Home',
        component: HomeView
    },
    {
        path: '/about',
        name: 'About',
        component: AboutView
    }
];

const router = createRouter({
    history: createWebHistory(),
    routes,
    linkActiveClass: "active",
});

export default router;