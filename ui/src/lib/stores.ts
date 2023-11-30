// https://dev.to/danawoodman/svelte-quick-tip-connect-a-store-to-local-storage-4idi
import { writable } from "svelte/store";

// ramp, target, hold
export interface Step extends Array<number>{
  0: number;
  1: number;
  2: number;
}

// named list containing all steps of a schedule
export interface Schedule extends Array<Step> {
  [index: number]: Step;
}

// list containing all schedules
export interface Schedules {
  [index: string]: Schedule;
}

export const stored_schedules = writable<Schedules>(
  JSON.parse(localStorage.getItem('schedules') || '{}')
)
stored_schedules.subscribe((value) => localStorage.schedules = JSON.stringify(value))

// logs
export interface RecordConfig {
  type: string;
  level: string;
  tag: string;
  detail: string;
  when: string;
}
export const stored_logs = writable<Array<RecordConfig>>([]);

// current kiln state
export interface KilnState {
  step: number;
  runtime: number;
  temperature: number;
  schedule: {
    name: string,
    steps: Schedule
  };
}
export const current_state = writable<KilnState>(JSON.parse('{}'));
