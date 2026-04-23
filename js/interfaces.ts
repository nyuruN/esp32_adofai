
export interface Event {
    floor: number,
    eventType: string,
    angleOffset?: number,

    beatsPerMinute?: number,
    speedType?: string,
    bpmMultiplier?: number,
    duration?: number,
    position?: (number | null)[],
    relativeTo?: string,
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
        bgImage: string,
        songFilename: string,
        beatsAhead: number,
        bpm: number,
        relativeTo: string,
        position: number[],
        rotation: number,
        zoom: number,
        offset: number, // mili second wait time till first beat
    },
    actions: Event[],
    angleData: number[],
    pathData?: string,
}

// Parser from
// https://github.com/adofaiex/ADOFAI-JS.git

interface PathDataTable {
    [key: string]: number;
}

const pathDataTable: PathDataTable = { "R": 0, "p": 15, "J": 30, "E": 45, "T": 60, "o": 75, "U": 90, "q": 105, "G": 120, "Q": 135, "H": 150, "W": 165, "L": 180, "x": 195, "N": 210, "Z": 225, "F": 240, "V": 255, "D": 270, "Y": 285, "B": 300, "C": 315, "M": 330, "A": 345, "5": 555, "6": 666, "7": 777, "8": 888, "!": 999 };
const parseToangleData = (pathdata: string): number[] =>
    Array.from(pathdata).map(t => pathDataTable[t as keyof typeof pathDataTable]);

export {
    pathDataTable,
    parseToangleData
}