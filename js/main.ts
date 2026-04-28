
import * as fflate from 'fflate'

// @ts-ignore
import { default as createModule } from '../build/index.js';

import { Event, AdofaiFile, parseToangleData } from './interfaces.js';

var canvas = document.getElementById('canvas') as HTMLCanvasElement;
var moduleLoaded = false;
var module: any;
var beatmaps: AdofaiFile[] = [];
var archive: fflate.Unzipped | null = null;

createModule({
	print: function (text: string) {
		console.log("log: " + text)
	},
	printErr: function (text: string) {
		console.error(text)
	},
	canvas: (function () {
		return canvas;
	})(),
}).then((e: any) => {
	module = e
	moduleLoaded = true
});

function update_beatmap_options() {
	var container = document.getElementsByClassName('container')[0]

	beatmaps.forEach((beatmap) => {
		var div = document.createElement('div')
		div.classList.add("beatmap-listing")
		var title = document.createElement('h1')
		title.innerHTML = beatmap.settings.song
		div.appendChild(title)
		var artist = document.createElement('h2')
		artist.textContent = beatmap.settings.artist
		div.appendChild(title)
		var levelDesc = document.createElement('p')
		levelDesc.textContent = beatmap.settings.levelDesc
		div.appendChild(levelDesc)

		container.appendChild(div)
	})
}

async function getImage(path: string) {
	if (!archive) return;

	let extension = path.split('.').reverse()[0]
	let imageBlob = new Blob([archive[path].buffer as ArrayBuffer], { type: 'image/' + extension })
	let url = URL.createObjectURL(imageBlob)

	let img = new Image()
	await new Promise<void>((resolve) => {
		img.onload = () => resolve();
		img.src = url;
	})
	URL.revokeObjectURL(url)

	const canvas = document.createElement('canvas') as HTMLCanvasElement;
	const ctx = canvas.getContext('2d') as CanvasRenderingContext2D;
	canvas.width = 320
	canvas.height = 240
	const ar = img.width / img.height
	const x0 = (canvas.height * ar - canvas.width) * 0.5;
	ctx.drawImage(img, -x0, 0, canvas.height * ar, 240)

	/* Quantize image
	let imgData = ctx.getImageData(0, 0, 320, 240);
	let data = imgData.data;
	for (let i = 0; i < data.length; i++) {
		data[i * 4] = Math.round(data[i * 4] / 3) * 3;
		data[i * 4 + 1] = Math.round(data[i * 4 + 1] / 3) * 3;
		data[i * 4 + 2] = Math.round(data[i * 4 + 2] / 2) * 2;
	}
	*/

	let blob = await new Promise<Blob | null>((resolve) => {
		canvas.toBlob(bb => resolve(bb), "image/jpeg", 0.95)
	})
	
	if (blob === null) return

	let buffer = new Uint8Array(await blob.arrayBuffer());
	let imgPtr = module._malloc(buffer.byteLength)
	let imgHeapView = new Uint8Array(module.HEAPU8.buffer, imgPtr, buffer.byteLength)
	imgHeapView.set(buffer)
	module._set_background_jpg(imgPtr, buffer.byteLength)

	/* Download image
	var a = document.createElement('a');
	a.download = 'img.jpg';
	a.href = window.URL.createObjectURL(blob);
	a.click();
	*/
}

let fileInput = document.getElementById('file-input') as HTMLInputElement
fileInput.addEventListener('change', async (ev) => {
	if (fileInput.files) {
		if (fileInput.files[0].name.endsWith(".zip")) {
			let data = await fileInput.files[0].bytes()
			fflate.unzip(data, (err, data) => {
				if (err) {
					console.error(err)
					return
				}
				archive = data
				Object.keys(data).forEach(key => {
					if (key.endsWith(".adofai")) {
						let decoder = new TextDecoder()
						let text = decoder.decode(data[key])
						let file = JSON.parse(text) as AdofaiFile
						beatmaps.push(file);
					}
				});

				//update_beatmap_options();
				
				serialize(beatmaps[0]);
			})
		} else if (fileInput.files[0].name.endsWith(".adofai")) {
			let text = await fileInput.files[0].text()
			serialize(JSON.parse(text) as AdofaiFile)
		}
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
	/*
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

		if (data.settings.bgImage != '') {
			getImage(data.settings.bgImage)
		}

		module._play();
	}
}
