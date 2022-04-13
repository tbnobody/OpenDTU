import { createWebHistory, createRouter } from 'vue-router';
import HomeView from '@/components/HomeView'
import AboutView from '@/components/AboutView'
import NetworkInfoView from '@/components/NetworkInfoView'

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
    },
    {
        path: '/info/network',
        name: 'Network',
        component: NetworkInfoView
    }
];

const router = createRouter({
    history: createWebHistory(),
    routes,
    linkActiveClass: "active",
});

export default router;