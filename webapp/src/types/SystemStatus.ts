export interface TaskDetail {
    name: string;
    stack_watermark: number;
    priority: number;
}

export interface SystemStatus {
    // HardwareInfo
    chipmodel: string;
    chiprevision: number;
    chipcores: number;
    cpufreq: number;
    cputemp: number;
    flashsize: number;
    // TaskDetails
    task_details: TaskDetail[];
    // FirmwareInfo
    hostname: string;
    sdkversion: string;
    config_version: string;
    git_hash: string;
    git_is_hash: boolean;
    git_branch: string;
    pioenv: string;
    resetreason_0: string;
    resetreason_1: string;
    cfgsavecount: number;
    uptime: number;
    update_text: string;
    update_url: string;
    update_status: string;
    // MemoryInfo
    heap_total: number;
    heap_used: number;
    heap_max_block: number;
    heap_min_free: number;
    littlefs_total: number;
    littlefs_used: number;
    psram_total: number;
    psram_used: number;
    sketch_total: number;
    sketch_used: number;
    // RadioInfo
    nrf_configured: boolean;
    nrf_connected: boolean;
    nrf_pvariant: boolean;
    cmt_configured: boolean;
    cmt_connected: boolean;
}
