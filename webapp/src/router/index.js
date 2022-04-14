import { createWebHistory, createRouter } from 'vue-router';
import HomeView from '@/components/HomeView'
import AboutView from '@/components/AboutView'
import NetworkInfoView from '@/components/NetworkInfoView'
import SystemInfoView from '@/components/SystemInfoView'

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
    },
    {
        path: '/info/system',
        name: 'System',
        component: SystemInfoView
    }
];

const router = createRouter({
    history: createWebHistory(),
    routes,
    linkActiveClass: "active",
});

export default router;