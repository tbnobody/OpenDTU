export interface HttpRequestConfig {
    url: string;
    auth_type: number;
    username: string;
    password: string;
    header_key: string;
    header_value: string;
    timeout: number;
}
