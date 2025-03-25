export interface HistoricalDataPoint {
  timestamp: number;
  yield: number;
  peak_power: number;
}

export interface HistoricalDataInfo {
  resolution: number;
  start_time: number;
  end_time: number;
  point_count: number;
}

export interface HistoricalData {
  info: HistoricalDataInfo;
  timestamps: number[];
  yields: number[];
  peak_powers: number[];
}
