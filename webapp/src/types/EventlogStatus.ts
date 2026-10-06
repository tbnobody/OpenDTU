export interface EventlogItem {
    message_id: number;
    message: string;
    start_time: number;
    end_time: number;
}

export interface EventlogItems {
    count: number;
    reported_count: number;
    last_request_status: 'ok' | 'pending' | 'failure';
    events: Array<EventlogItem>;
}
