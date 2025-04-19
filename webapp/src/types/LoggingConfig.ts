export interface LogModule {
    name: string;
    level: number;
}

export interface LogLevel {
    default: number;
    modules: Array<LogModule>;
}

export interface LoggingConfig {
    loglevel: LogLevel;
}
