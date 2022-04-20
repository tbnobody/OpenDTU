import { createWebHistory, createRouter } from 'vue-router';
import HomeView from '@/components/HomeView'
import AboutView from '@/components/AboutView'
import NetworkInfoView from '@/components/NetworkInfoView'
import SystemInfoView from '@/components/SystemInfoView'
import NtpInfoView from '@/components/NtpInfoView'
import NetworkAdminView from '@/components/NetworkAdminView'
import NtpAdminView from '@/components/NtpAdminView'
import MqttAdminView from '@/components/MqttAdminView'

const routes = [{
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
    },
    {
        path: '/info/ntp',
        name: 'NTP',
        component: NtpInfoView
    },
    {
        path: '/settings/network',
        name: 'Network Settings',
        component: NetworkAdminView
    },
    {
        path: '/settings/ntp',
        name: 'NTP Settings',
        component: NtpAdminView
    },
    {
        path: '/settings/mqtt',
        name: 'MqTT Settings',
        component: MqttAdminView
    }
];

const router = createRouter({
    history: createWebHistory(),
    routes,
    linkActiveClass: "active",
});

export default router;