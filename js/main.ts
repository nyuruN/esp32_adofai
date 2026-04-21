
import * as fflate from 'fflate'

// @ts-ignore
import { default as createModule } from '../build/index.js';

import { Event, AdofaiFile, parseToangleData } from './interfaces.js';



var canvas = document.getElementById('canvas') as HTMLCanvasElement;

var moduleLoaded = false;
var module: any;
createModule({
    print: function (text: string) {
        console.log("log: " + text);
    },
    printErr: function (text: string) {
        console.error(text);
    },
    canvas: (function () {
        return canvas;
    })(),
}).then((e: any) => {
    module = e
    moduleLoaded = true
});

let fileInput = document.getElementById('file-input') as HTMLInputElement
fileInput.addEventListener('change', async (ev) => {
    if (fileInput.files) {
        let text = await fileInput.files[0].text()
        serialize(JSON.parse(text) as AdofaiFile)
    }
})
let clearBtn = document.getElementById('clear-btn') as HTMLButtonElement
clearBtn.addEventListener('click', (ev) => {
    if (moduleLoaded) {
        module._clear(false)
        module._play()
    }
})
document.addEventListener('keydown', (ev) => {
    if (moduleLoaded) {
        module._hit()
    }
})

function getEventType(e: Event): number {
    if (e.eventType == 'CameraOffset') {
        return 0
    }
    if (e.eventType == 'CameraZoom') {
        return 1
    }
    if (e.eventType == 'CameraRotate') {
        return 2
    }
    if (e.eventType == 'ShakeScreen') {
        return 3
    }
    if (e.eventType == 'SetSpeed') {
        return 4
    }
    if (e.eventType == 'CameraSetMode') {
        return 5
    }

    throw new Error('Invalid event type: \"' + e.eventType + '\"')
}
function serialize(data: AdofaiFile) {
    const TileData = {
        Twirl: 1,
        SpeedUp: 2,
        SpeedDown: 4,
        Checkpoint: 8
    }

    // Process PathData
    if (data.pathData !== undefined) {
        data.angleData = parseToangleData(data.pathData)
    }

    let tileCount = data.angleData.length
    let tileData = new Uint8Array(data.angleData.length)
    let events: Event[] = []
    let bpm = data.settings.bpm

    // Process events
    data.actions.forEach((v) => {
        if (v.eventType == 'Twirl') {
            tileData[v.floor] |= TileData.Twirl
            return false;
        }
        if (v.eventType == 'Checkpoint') {
            tileData[v.floor] |= TileData.Checkpoint
            return false;
        }
        if (v.eventType == 'SetSpeed') {
            if (v.speedType == 'Multiplier') {
                if ((v.bpmMultiplier as number) > 1) {
                    tileData[v.floor] |= TileData.SpeedUp
                } else {
                    tileData[v.floor] |= TileData.SpeedDown
                }
                bpm *= v.bpmMultiplier as number
            } else {
                if (v.beatsPerMinute as number > bpm) {
                    tileData[v.floor] |= TileData.SpeedUp
                } else {
                    tileData[v.floor] |= TileData.SpeedDown
                }
                bpm = v.beatsPerMinute as number
            }

            v.beatsPerMinute = bpm
            events.push(v)
        }
        if (v.eventType == 'ShakeScreen') {
            events.push(v)
        }
        if (v.eventType == 'MoveCamera') {
            // Prevent repeatedly setting camera mode, unless camera mode is Tile
            if (v.relativeTo !== undefined) {
                let e: Event = { ...v }
                e.eventType = 'CameraSetMode'
                events.push(e)
            }
            if (v.zoom !== undefined) {
                let e: Event = { ...v }
                e.eventType = 'CameraZoom'
                events.push(e)
            }
            if (v.rotation !== undefined) {
                let e: Event = { ...v }
                e.eventType = 'CameraRotate'
                events.push(e)
            }
            if (v.position !== undefined && (v.position[0] !== null || v.position[1] !== null)) {
                let e: Event = { ...v }
                e.eventType = 'CameraOffset'
                events.push(e)
            }
        }
    })

    const little_endian = true;

    const angle_size = 2
    let angleBuffer = new ArrayBuffer(tileCount * angle_size)
    let angleView = new DataView(angleBuffer)
    for (let i = 0; i < tileCount; i++) {
        let angle = data.angleData[i]
        if (angle < 0) { angle += 360 }
        angleView.setInt16(i * angle_size, angle, little_endian)
    }

    const event_size = 14;
    let eventBuffer = new ArrayBuffer(events.length * event_size)
    let eventView = new DataView(eventBuffer)

    // Serialize Events
    events.forEach((v, i) => {
        let offset = i * event_size

        eventView.setUint8(offset, getEventType(v))
        offset += 1
        eventView.setUint32(offset, v.floor, little_endian)
        offset += 4;
        eventView.setUint16(offset, v.angleOffset ? v.angleOffset : 0, little_endian)
        offset += 2;

        if (v.eventType == 'SetSpeed') {
            eventView.setFloat32(offset, v.beatsPerMinute as number, little_endian)
            console.log(v.beatsPerMinute as number)
        }
        if (v.eventType == 'ShakeScreen') {
            eventView.setUint16(offset, (v.duration as number) * 1000, little_endian)
            offset += 2
            eventView.setUint16(offset, (v.intensity as number), little_endian)
            offset += 2
            eventView.setUint16(offset, (v.strength as number), little_endian)
            offset += 2
        }
        if (v.eventType == 'CameraZoom') {
            eventView.setUint16(offset, (v.duration as number) * 1000, little_endian)
            offset += 2
            eventView.setFloat32(offset, (1 / ((v.zoom as number) / 100)), little_endian)
            offset += 4
        }
        if (v.eventType == 'CameraRotate') {
            eventView.setUint16(offset, (v.duration as number) * 1000, little_endian)
            offset += 2
            eventView.setFloat32(offset, (v.rotation as number), little_endian)
            offset += 4
        }
        if (v.eventType == 'CameraOffset' && v.position) {
            eventView.setUint16(offset, (v.duration as number) * 1000, little_endian)
            offset += 2
            eventView.setInt16(offset, (v.position[0] ? v.position[0] : 0) * 1000, little_endian)
            offset += 2
            eventView.setInt16(offset, (v.position[1] ? v.position[1] : 0) * 1000, little_endian)
            offset += 2
        }
        if (v.eventType == 'CameraSetMode' && v.relativeTo) {
            eventView.setUint16(offset, (v.duration as number) * 1000, little_endian)
            offset += 2
            eventView.setUint8(offset, (v.relativeTo === 'Player' ? 0 : 1))
            offset += 1
        }
    })

    // Debug
    let angleString = '';
    (new Uint8Array(angleBuffer)).forEach((v) => {
        angleString += '0x' + v.toString(16).padStart(2, '0') + ','
    })
    console.log('angle:\n' + angleString)
    let tileString = ''
    tileData.forEach((v) => {
        tileString += '0x' + v.toString(16).padStart(2, '0') + ','
    })
    console.log('tile:\n' + tileString)
    let eventString = '';
    (new Uint8Array(eventBuffer)).forEach((v) => {
        eventString += '0x' + v.toString(16).padStart(2, '0') + ','
    })
    console.log('event:\n' + eventString)
    console.log(events.length)

    // Output zip file
    /*
    const zipped = fflate.zipSync({
        'angleData.bin': new Uint8Array(angleBuffer),
        'tileData.bin': tileData,
        'eventData.bin': new Uint8Array(eventBuffer),
    })
    var bb = new Blob([zipped as Uint8Array<ArrayBuffer>], { type: 'application/octet-stream' });
    var a = document.createElement('a');
    a.download = 'data.zip';
    a.href = window.URL.createObjectURL(bb);
    a.click();
    */

    if (moduleLoaded) {
        let tilePtr = module._malloc(tileData.length)
        let tileHeapView = new Uint8Array(module.HEAPU8.buffer, tilePtr, tileData.length)
        tileHeapView.set(tileData);
        let angleData = new Uint8Array(angleBuffer)
        let anglePtr = module._malloc(angleData.length)
        let angleHeapView = new Uint8Array(module.HEAPU8.buffer, anglePtr, angleData.length)
        angleHeapView.set(angleData)
        let eventData = new Uint8Array(eventBuffer)
        let eventPtr = module._malloc(eventData.length)
        let eventHeapView = new Uint8Array(module.HEAPU8.buffer, eventPtr, eventData.length)
        eventHeapView.set(eventData)

        module._clear();
        module._set_beatmap_data(anglePtr, tilePtr, tileData.length);
        module._set_event_data(eventPtr, events.length);
        module._set_bpm(data.settings.bpm);
        module._play();
    }
}

/*
function serialize(json: JSON) {
    let data = json as unknown as AdofaiFile 
    console.log(data)

    let angleData = ''
    data.angleData.forEach((v, i) => {
        angleData += v + ','
    })

    console.log(angleData)

    let eventData = ''
    let twirlData = Array<boolean>(data.angleData.length)
    let speedUpData = Array<boolean>(data.angleData.length)
    let speedDownData = Array<boolean>(data.angleData.length)

    let eventCount = 0

    data.actions.forEach((v, i) => {
        if (v.eventType == 'Twirl') {
            twirlData[v.floor] = true
            return
        }

        if (v.eventType == 'SetSpeed') {
            if ((v.bpmMultiplier as number) > 1) {
                speedUpData[v.floor] = true
            } else {
                speedDownData[v.floor] = true
            }
            eventCount++
            eventData +=
`Event {
    .type = EventType::SetSpeed,
    .floor = ${v.floor},
    .angle_offset = ${v.angleOffset ? v.angleOffset : 0},
    .set_speed = SetSpeed {
    .bpm = ${v.beatsPerMinute as number},
    },
},\n`
        }
        if (v.eventType == 'ShakeScreen') {
            eventCount++
            eventData +=
`Event {
    .type = EventType::ShakeScreen,
    .floor = ${v.floor},
    .angle_offset = ${v.angleOffset ? v.angleOffset : 0},
    .shake_screen = ShakeScreen {
    .duration = ${((v.duration as number) * 1000).toFixed(0)},
    .intensity = ${((v.intensity as number)).toFixed(0)},
    .strength = ${((v.strength as number)).toFixed(0)},
    .ease = EaseType::Linear,
    },
},\n`
        }
        if (v.eventType == 'MoveCamera') {
            if (v.zoom) {
                eventCount++
                eventData +=`
Event {
    .type = EventType::CameraZoom,
    .floor = ${v.floor},
    .angle_offset = ${v.angleOffset ? v.angleOffset : 0},
    .camera_zoom = CameraZoom {
    .duration = ${((v.duration as number) * 1000).toFixed(0)},
    .zoom = ${((1 / (v.zoom as number / 100)) * 1000).toFixed(0)},
    .ease = EaseType::Linear,
    },
},`
            }
            if (v.rotation) {
                eventCount++
                eventData +=`
Event {
    .type = EventType::CameraRotation,
    .floor = ${v.floor},
    .angle_offset = ${v.angleOffset ? v.angleOffset : 0},
    .camera_rotation = CameraRotation {
    .duration = ${((v.duration as number) * 1000).toFixed(0)},
    .rotation = ${((v.rotation as number)).toFixed(0)},
    .ease = EaseType::Linear,
    },
},`
            }
            if (v.position && (v.position[0] || v.position[1])) {
                eventCount++
                eventData +=`
Event {
    .type = EventType::CameraOffset,
    .floor = ${v.floor},
    .angle_offset = ${v.angleOffset ? v.angleOffset : 0},
    .camera_offset = CameraOffset {
    .duration = ${((v.duration as number) * 1000).toFixed(0)},
    .offset_x = ${((v.position[0] ? v.position[0] : 0) * 1000).toFixed(0)},
    .offset_y = ${((v.position[0] ? v.position[0] : 0) * 1000).toFixed(0)},
    .ease = EaseType::Linear,
    },
},`
            }
        }
    })

    console.log(eventData)
    console.log(eventCount)

    let tile_data = ''
    
    for (let i = 0; i < data.angleData.length; i++) {
        let twirl = twirlData[i]
        let spdUp = speedUpData[i]
        let spdDown = speedDownData[i]

        if (twirl) {
            tile_data += 'TILE_TWIRL,'
        } else if (spdDown) {
            tile_data += 'TILE_SPEEDDOWN,'
        } else if (spdUp) {
            tile_data += 'TILE_SPEEDUP,'
        } else {
            tile_data += '0,'
        }
    }

    console.log(tile_data)
}
*/