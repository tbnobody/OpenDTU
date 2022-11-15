export interface EventlogItem {
    message_id: number;
    message: string;
    start_time: number;
    end_time: number;
}

export interface EventlogItems {
    count: number;
    events: Array<EventlogItem>;
}