
#include "rust_typedef.h"
#include "lgfx.h"
#include "events.h"
#include "store.h"
#include "music_player.h"
#include "debug.h"

#if defined(ESP_PLATFORM)

#include <Wifi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const char *ssid = "MySSID";			   // Enter SSID here
const char *password = "MyPasswd"; // Enter Password here

inline WebServer server(80);

enum class NetworkState
{
	Disconnected,
	Idle,
	Connect,
	Connecting,
	Connected,
};

inline NetworkState network_state = NetworkState::Connect;

namespace Upload
{
	inline u8 *buf = nullptr;
	inline i32 len;
	inline i32 written;

	inline void handle_upload()
	{
		HTTPUpload &upload = server.upload();

		switch (upload.status)
		{
		case UPLOAD_FILE_START:
			len = server.clientContentLength();
			buf = (u8 *)heap_caps_malloc(len, MALLOC_CAP_SPIRAM);
			written = 0;
			if (buf)
				printf("Upload started: %s (%d bytes)\n", upload.filename.c_str(), len);
			else
				printf("SPIRAM memory allocation failed!\n");
			break;
		case UPLOAD_FILE_WRITE:
			if (!buf)
				break;
			memcpy(buf + written, upload.buf, upload.currentSize);
			written += upload.currentSize;
			printf("Received %d bytes (total: %d)\n", upload.currentSize, upload.totalSize);
			break;

		case UPLOAD_FILE_END:
			printf("Upload Success\n");
			break;
		}
	}
};

#include "beatmap.h"

namespace BeatmapUpload
{
	inline u8 *buf = nullptr;
	inline i32 len;
	inline i32 written;

	Beatmap beatmap;
	u8 upload_state;

	struct BeatmapUploadHeader {

	} __attribute__((packed));

	inline void handle_upload_finish()
	{
		if (!buf) {
			server.send(400, "text/plain", "memory allocation failed");
			return;
		}

		printf("Beatmap uploaded: %p (%d bytes)\n", (void *)buf, len);

		beatmap = Beatmap();

		beatmap.mallocBuf = buf;

		/* I don't understand why you won't parse arguments when in raw mode
		beatmap.tileCount = server.arg(0).toInt();
		beatmap.eventCount = server.arg(1).toInt();
		beatmap.bgSize = server.arg(2).toInt();
		beatmap.songSize = server.arg(3).toInt();
		*/
		beatmap.tileCount = 	*((u32 *)(buf) + 0);
		beatmap.eventCount = 	*((u32 *)(buf) + 1);
		beatmap.bgSize = 		*((u32 *)(buf) + 2);
		beatmap.songSize = 		*((u32 *)(buf) + 3);
		beatmap.startBpm = 		*((float *)(buf) + 4);
		beatmap.totalSize += 20; // Settings

		beatmap.tileData = (u8 *)(buf + beatmap.totalSize);
		beatmap.totalSize += beatmap.tileCount; // tileData

		beatmap.angleData = (u16 *)(buf + beatmap.totalSize);
		beatmap.totalSize += beatmap.tileCount * 2; // angleData

		beatmap.eventData = (Event *)(buf + beatmap.totalSize);
		beatmap.totalSize += beatmap.eventCount * 14; // eventData

		if (beatmap.bgSize != 0)
			beatmap.bgData = (u8 *)(buf + beatmap.totalSize);
		beatmap.totalSize += beatmap.bgSize;

		beatmap.songData = (u8 *)(buf + beatmap.totalSize);
		beatmap.totalSize += beatmap.songSize;

		if (beatmap.totalSize != len)
		{
			printf("calculated size %d does not match provided data %d!\n", beatmap.totalSize, len);
			server.send(400, "text/plain", "incorrect content length!");
			beatmap.destroy();
			return;
		}

		beatmapStore.add(beatmap);
		server.send(200, "text/plain", "did well here");

		BeatmapPlayer::set_beatmap_data((u8*)beatmap.angleData, beatmap.tileData, beatmap.tileCount);
		BeatmapPlayer::set_event_data((u8*)beatmap.eventData, beatmap.eventCount);
		BeatmapPlayer::set_bpm(beatmap.startBpm);
		if (beatmap.bgData)
			BeatmapPlayer::background.drawJpg(beatmap.bgData, beatmap.bgSize);
		BeatmapPlayer::clear();
		BeatmapPlayer::begin();
		music_player.set_song(beatmap.songData, beatmap.songSize);
		music_player.play();
		print_memory_info();
	}

	inline void handle_upload()
	{
		HTTPRaw &upload = server.raw();

		switch (upload.status)
		{
		case RAW_START:
			len = server.clientContentLength();
			if (len > beatmapStore.get_available_size())
				beatmapStore.clear();
			buf = (u8 *)heap_caps_malloc(len, MALLOC_CAP_SPIRAM);
			printf("Buffer %p\n", buf);
			written = 0;
			if (buf)
				printf("Upload started: %d (%d bytes)\n", upload.totalSize, len);
			else
			{
				printf("SPIRAM memory allocation failed!\n");
				break;
			}

			break;

		case RAW_WRITE:
			if (!buf)
				break;
			memcpy(buf + written, upload.buf, upload.currentSize);
			written += upload.currentSize;
			printf("Received %d bytes (total: %d)\n", upload.currentSize, upload.totalSize);

			break;

		case RAW_END:
			printf("Raw end\n");
			break;
		}
	}
};

inline void setup_webserver()
{
	server.enableCORS();

	server.on("/", HTTP_GET, []
			  { server.send(200, "text/html", "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"></head><body><h1>Hey there!</h1></body></html>"); });

	server.onNotFound([]
					  { server.send(404, "text/plain", "Not found"); });

	server.on("/setup-wifi", HTTP_GET, []
			  {
        auto ssid = server.arg(0);
				
        auto passwd = server.arg(1);
        printf("Received password q0=%s, q1=%s\n", ssid, passwd);
        server.send(200, "text/plain", ":)"); });

	server.on("/clear", HTTP_GET, []
			  { beatmapStore.clear(); server.send(200); });

	server.on("/upload-beatmap", HTTP_POST, BeatmapUpload::handle_upload_finish, BeatmapUpload::handle_upload);
	
	server.on("/upload", HTTP_POST, []
			  {
							if (Upload::buf) {
								printf("Received data: %p (%d bytes)\n", (void*)Upload::buf, Upload::len);
								server.send(200);
								free((void*)Upload::buf);
							} else {
								server.send(400);
							} }, Upload::handle_upload);
}
inline void network_update()
{
	static constexpr u32 retry_frames = 180; // Frames between retries
	static constexpr u32 retry_max = 3;		 // Maximum number of retries
	static u8 retry_count;
	static u32 retry_timer;

	switch (network_state)
	{
	case NetworkState::Disconnected:
		break;
	case NetworkState::Idle:
		server.handleClient();
		break;
	case NetworkState::Connected:
		if (!MDNS.begin("adofai"))
		{
			printf("mDNS setup failed!\n");
		}
		server.begin();
		network_state = NetworkState::Idle;
		break;
	case NetworkState::Connect:
		WiFi.begin(ssid, password);
		printf("WiFi Begin Status: %d\n", WiFi.status());
		network_state = NetworkState::Connecting;
		retry_count = 0;
		retry_timer = 0;
		break;
	case NetworkState::Connecting:
		if (WiFi.status() == WL_CONNECTED)
			network_state = NetworkState::Connected;
		else if (WiFi.status() != WL_IDLE_STATUS)
		{
			printf("WiFi Status: %d\n", WiFi.status());
			network_state = NetworkState::Connect;
		}
		else
		{
			retry_timer++;
			if (retry_timer > retry_frames)
			{
				retry_timer = 0;
				network_state = (retry_count++ < retry_max) ? NetworkState::Connect : NetworkState::Disconnected;
			}
		}
		break;
	}
}

#else

inline void setup_webserver() {}
inline void network_update() {}

#endif