import { createRouter, createWebHistory } from 'vue-router';
import HomeView from '@/views/HomeView.vue'
import AboutView from '@/views/AboutView.vue'
import NetworkInfoView from '@/views/NetworkInfoView.vue'
import SystemInfoView from '@/views/SystemInfoView.vue'
import NtpInfoView from '@/views/NtpInfoView.vue'
import NetworkAdminView from '@/views/NetworkAdminView.vue'
import NtpAdminView from '@/views/NtpAdminView.vue'
import MqttAdminView from '@/views/MqttAdminView.vue'
import MqttInfoView from '@/views/MqttInfoView.vue'
import InverterAdminView from '@/views/InverterAdminView.vue'
import DtuAdminView from '@/views/DtuAdminView.vue'
import FirmwareUpgradeView from '@/views/FirmwareUpgradeView.vue'
import ConfigAdminView from '@/views/ConfigAdminView.vue'
import SecurityAdminView from '@/views/SecurityAdminView.vue'
import LoginView from '@/views/LoginView.vue'
import MaintenanceRebootView from '@/views/MaintenanceRebootView.vue';

const router = createRouter({
    history: createWebHistory(import.meta.env.BASE_URL),
    linkActiveClass: "active",
    routes: [
    {
        path: '/',
        name: 'Home',
        component: HomeView
    },
    {
        path: '/login',
        name: 'Login',
        component: LoginView
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
    },
    {
        path: '/settings/config',
        name: 'Config Management',
        component: ConfigAdminView
    },
    {
        path: '/settings/security',
        name: 'Security',
        component: SecurityAdminView
    },
    {
        path: '/maintenance/reboot',
        name: 'Device Reboot',
        component: MaintenanceRebootView
    }
]
});

export default router;