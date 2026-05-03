import { ALL_FORMATS, BufferSource, BufferTarget, Conversion, Input, Mp3OutputFormat, OggOutputFormat, Output, Quality, QUALITY_LOW } from "mediabunny/dist/modules/src";

export async function encodeAudio(buffer: ArrayBuffer) {
  const input = new Input({
    formats: ALL_FORMATS,
    source: new BufferSource(buffer),
  });

  const output = new Output({
    format: new OggOutputFormat(),
    target: new BufferTarget(),
  })

  const conversion = await Conversion.init({ input, output, audio: {
    codec: 'opus',
    bitrate: 96e3,
  }});

  if (!conversion.isValid) {
    console.log(conversion.discardedTracks);
    return
  }

  await conversion.execute()

  return output.target.buffer as ArrayBuffer
}