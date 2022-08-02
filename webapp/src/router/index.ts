import { createWebHistory, createRouter, RouteRecordRaw } from 'vue-router';
import HomeView from '@/components/HomeView.vue'
import AboutView from '@/components/AboutView.vue'
import NetworkInfoView from '@/components/NetworkInfoView.vue'
import SystemInfoView from '@/components/SystemInfoView.vue'
import NtpInfoView from '@/components/NtpInfoView.vue'
import NetworkAdminView from '@/components/NetworkAdminView.vue'
import NtpAdminView from '@/components/NtpAdminView.vue'
import MqttAdminView from '@/components/MqttAdminView.vue'
import MqttInfoView from '@/components/MqttInfoView.vue'
import InverterAdminView from '@/components/InverterAdminView.vue'
import DtuAdminView from '@/components/DtuAdminView.vue'
import FirmwareUpgradeView from '@/components/FirmwareUpgradeView.vue'
import VedirectAdminView from '@/components/VedirectAdminView.vue'
import VedirectInfoView from '@/components/VedirectInfoView.vue'

const routes: Array<RouteRecordRaw> = [
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
    },
    {
        path: '/info/ntp',
        name: 'NTP',
        component: NtpInfoView
    },
    {
        path: '/info/mqtt',
        name: 'MqTT',
        component: MqttInfoView
    },
    {
        path: '/info/vedirect',
        name: 'Ve.direct',
        component: VedirectInfoView
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
        path: '/settings/vedirect',
        name: 'Ve.direct Settings',
        component: VedirectAdminView
    },
    {
        path: '/settings/mqtt',
        name: 'MqTT Settings',
        component: MqttAdminView
    },
    {
        path: '/settings/inverter',
        name: 'Inverter Settings',
        component: InverterAdminView
    },
    {
        path: '/settings/dtu',
        name: 'DTU Settings',
        component: DtuAdminView
    },
    {
        path: '/firmware/upgrade',
        name: 'Firmware Upgrade',
        component: FirmwareUpgradeView
    }
];

const router = createRouter({
    history: createWebHistory(),
    routes,
    linkActiveClass: "active",
});

export default router;