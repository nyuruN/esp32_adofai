
export interface Event {
    floor: number,
    eventType: string,
    angleOffset?: number,

    beatsPerMinute?: number,
    speedType?: string,
    bpmMultiplier?: number,
    duration?: number,
    position?: (number | null)[],
    zoom?: number,
    ease?: string,
    fadeOut?: boolean,
    intensity?: number,
    strength?: number,
    rotation?: number,
}
export interface AdofaiFile {
    settings: {
        artist: string,
        backgroundColor: number,
        beatsAhead: number,
        bpm: number,
    },
    actions: Event[],
    angleData: number[],
}