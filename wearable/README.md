# Jarvis

This guide is for the `wearable` folder. `brain` and `firmware` sit next to this file. Run the steps below from here, not from the repository root.

A small personal assistant in two pieces:

1. **The watch** is a Waveshare ESP32-S3-Touch-AMOLED-1.75C. It draws a round animated face, offers a few tap-to-send phrases, and can record a few seconds from the microphones.
2. **The brain** is a small Python program on your Windows laptop. The watch sends it text (or a recording). The brain asks a cloud model, remembers a short conversation, and sends a short reply back. The watch shows that reply.

The laptop does not run the model itself. It only forwards messages. If you have not added an API key yet, the brain answers in **practice mode** so you can still test everything.

This is phase 1. Bluetooth fallback, Tailscale, and spoken replies are planned and have a place in the code, but they are not built yet.

## What you need

| Item | Notes |
| --- | --- |
| Waveshare ESP32-S3-Touch-AMOLED-1.75C | SKU 33691 includes a battery. SKU 33692 does not. Either one is fine. |
| USB-C cable that carries data | A charge-only cable will power the watch and then fail when you try to flash it. |
| Windows laptop | The brain runs here. It does not need a powerful GPU. |
| Wi-Fi | The laptop and the watch join the same network. |
| Phone | Used once, to type the Wi-Fi name and the laptop address into the watch. |
| Optional 3.7 V battery | MX1.25 2-pin plug, only if your board did not come with one. USB power is enough for the first tests. |

Official pages, which this project follows for pins and chips:

- Product wiki: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C
- Example code and schematic: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C

The screen is a 466×466 AMOLED (chip CO5300) with a CST9217 touch controller. The microphones go through an ES7210 chip. The speaker codec (ES8311) is on the board and is left for a later phase. Power is an AXP2101. Motion is a QMI8658. The chip is an ESP32-S3R8 with 8 MB of PSRAM and 32 MB of flash.

## How a message travels

```
Watch  --Wi-Fi, on your home network-->  Laptop brain  --internet-->  Cloud model
                                              |
                                         practice mode
                                         if no API key
```

The watch never talks to the cloud directly. Only the laptop does. That keeps the API key off the watch.

## Part 1. Set up the brain on Windows

Do this before the watch arrives. You can have a conversation in the browser.

### Install Python

1. Go to https://www.python.org/downloads/ and download Python 3 for Windows.
2. Run the installer. On the first screen, tick **Add python.exe to PATH**. Then click Install.
3. Open **PowerShell** (search for it in the Start menu) and run:

```powershell
python --version
```

You should see Python 3.something. If Windows says it cannot find python, the PATH tick was missed. Run the installer again and choose **Modify**, then tick **Add python.exe to PATH**.

### Install and start Jarvis Brain

1. Clone this repository onto the laptop and open the `wearable` folder.
2. Open PowerShell in `wearable/brain` (the `brain` folder next to this file). The easy way: open that folder in File Explorer, click the address bar, type `powershell`, and press Enter.
3. Allow scripts for this window only, then install:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\setup.ps1
```

If PowerShell is awkward, double-click `setup.bat` instead. It does the same job.

4. Start the brain with `.\run.ps1`, or double-click `run.bat`.

The browser assistant in the rest of this repository also uses port 8787. Do not run both at the same time. If that port is taken, set `JARVIS_PORT=8788` in `brain/.env` and use `:8788` everywhere below.

Leave that window open. It will print two addresses:

- **On this laptop:** http://127.0.0.1:8787
- **From the watch:** http://192.168.x.x:8787 (your number will differ)

The first time, Windows Firewall may ask about Python. Allow it on **private** networks.

5. On the laptop, open a browser to http://127.0.0.1:8787

Type a sentence and press Send. In practice mode the reply says so, and it still proves the page, the memory, and the reply path work. The Morning, Joke, and Focus buttons send the same phrases the watch will use.

A second check, with the brain window still open, from another PowerShell in the `brain` folder:

```powershell
.\.venv\Scripts\python.exe -m jarvis_brain.cli Good morning, Jarvis
```

And, with the brain **not** required to be running:

```powershell
.\.venv\Scripts\python.exe -m jarvis_brain.selftest
```

`test.bat` runs that same self-test.

### Give it a real model

1. In the `brain` folder, open the file named `.env` in Notepad. (`setup` copied it from `.env.example`.)
2. Put your key on this line, with no quotes:

```
LLM_API_KEY=sk-your-key-here
```

3. Save. Stop the brain window with Ctrl+C, then start `run.bat` again.

The default model is `gpt-4o-mini` at `https://api.openai.com/v1`. To use another service that speaks the same API, change these lines and restart:

```
LLM_BASE_URL=https://api.groq.com/openai/v1
LLM_MODEL=llama-3.3-70b-versatile
LLM_API_KEY=your-groq-key
```

A model running on this same laptop through [Ollama](https://ollama.com/) looks like this. Ollama does not check the key, but the line cannot be blank or Jarvis stays in practice mode:

```
LLM_PROVIDER=local
LLM_BASE_URL=http://127.0.0.1:11434/v1
LLM_MODEL=llama3.2
LLM_API_KEY=local
```

To force practice mode even when a key is filled in, set `LLM_PROVIDER=mock`.

The personality is in `brain/jarvis_brain/persona.py`. It asks for one to three short sentences because the watch screen is small. You can point `JARVIS_PERSONA_FILE` in `.env` at your own text file instead of editing the Python.

## Part 2. Put Jarvis on the watch

Do this when the board is in your hands. You need Visual Studio Code and the PlatformIO extension. That is the path this project was built and compiled with.

### Install the tools

1. Install Visual Studio Code: https://code.visualstudio.com/
2. Open it. On the left, click Extensions (the square blocks icon). Search for **PlatformIO IDE**. Install it. The first install downloads a lot and can take a while.
3. The watch uses the ESP32-S3's own USB port, not a CH340 or CP2102 chip. Windows 10 and 11 usually install a port named **USB JTAG/serial debug unit** by themselves. If no port appears:
   - Try a different USB-C cable.
   - Hold the **BOOT** button while you plug the cable in, then let go. That only matters for the first flash if the chip is being stubborn. After Jarvis is installed, do not hold BOOT during plug-in; that is download mode.
   - Espressif's driver notes are here if Windows still shows nothing: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/establish-serial-connection.html

### Build and flash

1. In VS Code, choose **File → Open Folder** and open `wearable/firmware` (the `firmware` folder next to this file, not the repository root and not `brain`).
2. Wait until PlatformIO finishes its first setup. A small alien-head icon appears on the left when it is ready.
3. Click that icon, then **Project Tasks → waveshare_amoled_175c → General → Build**. The checkmark on the bottom bar does the same thing. The first build downloads libraries and can take several minutes. Later builds are quicker.
4. Plug in the watch. Click **Upload** (the arrow on the bottom bar). If upload cannot find the port, unplug the watch, hold **BOOT**, plug it back in, let go of BOOT, and click Upload again.
5. Click **Monitor** (the plug icon) to see the log. The speed is 115200. You should see `Jarvis watch, phase 1` and then either a Wi-Fi connection or instructions to join `Jarvis-Setup`.

From PowerShell, with `wearable` as the current folder, the same three steps are:

```powershell
cd firmware
pio run
pio run -t upload
pio device monitor
```

`pio` is installed by the PlatformIO extension. If PowerShell cannot find it, use the VS Code buttons instead.

### Join Wi-Fi and tell the watch where the laptop is

The watch has no Wi-Fi password stored in this project. The first time it boots, it opens its own network.

1. On your phone, join the Wi-Fi network named **Jarvis-Setup**.
2. A page should open on its own. If it does not, open a browser to http://192.168.4.1
   Turn mobile data off for this minute if the phone refuses to load the page. Use `http://`, not `https://`.
3. Pick your home Wi-Fi, type its password, and in **Laptop address** type the address the brain window printed, for example `http://192.168.1.20:8787`
4. Save. The watch restarts and joins your home Wi-Fi.

The brain window must be open, and the laptop must be on that same Wi-Fi. Guest networks that block devices from seeing each other will not work.

To change the Wi-Fi or the laptop address later: while Jarvis is already running, hold the **BOOT** button for about two seconds. The setup page comes back. Holding BOOT while the cable is being plugged in does something different (it enters flash mode), so only hold it once the face is on the screen.

### First conversation

On the face:

- **Morning, Joke, Focus, Question** send a prepared sentence. The chat page opens and the reply appears.
- **Talk** records for up to 4 seconds. Tap it again to send sooner. The laptop turns the audio into text, then answers. With no speech key, it uses a practice sentence so you can see the path work.
- **Chat** opens the last few lines without sending anything.

The phrases live in `firmware/include/prompts.h`. Change the words inside the quotes, then upload again. Keep the labels short.

With the serial monitor open you can also type:

```
say hello from the serial port
```

and press Enter. The watch sends that line to the laptop.

## What the microphone does

Tap **Talk**. The watch records mono audio at 16 kHz from the ES7210 microphone chip, wraps it as a WAV, and posts it to the brain at `/v1/voice`. The brain calls a speech-to-text API (the same style as OpenAI Whisper) and then the chat model.

Speech uses `STT_API_KEY` in `.env`, or `LLM_API_KEY` if the speech line is empty. `STT_MODEL` defaults to `whisper-1`. If neither key is set, the brain pretends it heard "Jarvis, are you there?" and answers in practice mode.

The speaker is not used yet. See the roadmap.

## If something goes wrong

**The brain page does not open.** The black window must still be running. Use http://127.0.0.1:8787 on the laptop. If the port is busy, change `JARVIS_PORT` in `.env` and restart. The watch must use the same port.

**Replies say practice mode.** `.env` has no `LLM_API_KEY`, or `LLM_PROVIDER` is `mock`. Fill in the key and restart the brain window.

**The model returns an error on the page.** The key, the model name, or `LLM_BASE_URL` does not match the service. The brain window prints the HTTP status. Fix the `.env` lines and restart.

**Upload says it cannot find a port.** Charge-only cable, or the port is held by the serial monitor. Close the monitor and upload again. Hold BOOT while plugging in, then release, then upload.

**The screen stays black.** Press nothing for ten seconds, then unplug and plug the USB cable back in. The face appears after Wi-Fi setup, which can take a little while on the first boot. If it is still black, open the serial monitor and look for `Display failed to start`.

**The face draws but taps do nothing.** The serial monitor will say if the touch chip did not answer. You can still type `say hello` in the monitor. If touch is mirrored the wrong way once you can tap, open `firmware/src/ui.cpp`, find `touch.setMirrorXY(true, true);`, and try `false, false`. That line matches Waveshare's touch demo.

**The watch joins Wi-Fi but says it cannot reach the laptop.** The brain window is closed, the laptop went to sleep, Windows Firewall blocked Python, or the address on the watch is wrong. On the laptop, run `ipconfig` and look for the **IPv4 Address** on the Wi-Fi adapter. It should match the address you typed, with `:8787` on the end. Hold BOOT for two seconds and enter it again.

**Jarvis-Setup never appears.** It only appears when the watch has no saved Wi-Fi, or when you hold BOOT for two seconds while Jarvis is running. Give the first boot a full minute.

**Talk says the microphone is not ready.** The ES7210 chip did not answer on the I2C bus. The buttons still work. The serial monitor has the exact line.

## Where things live

```
brain/                  the laptop program
  setup.ps1, run.ps1    Windows install and start (setup.bat and run.bat do the same)
  .env.example          every setting, with notes
  jarvis_brain/         the Python package
    server.py           HTTP and WebSocket
    persona.py          the system prompt
    memory.py           the short conversation memory
    llm/                model backends, including the Hermes plug-in
    stt/                speech-to-text
    tts/                speech-out stub for later
  static/index.html     the test page
firmware/               the watch program
  platformio.ini        board, libraries, and build settings
  include/pin_config.h  pin numbers copied from Waveshare
  include/prompts.h     the four buttons
  src/                  face, Wi-Fi portal, chat, microphone
  lib/es7210/           Waveshare's microphone driver, included so the build matches their demo
```

The watch calls the brain at these addresses:

| Address | What it is |
| --- | --- |
| `GET /health` | Is the brain up, and is it in practice mode? |
| `POST /v1/chat` | JSON `{"text", "session_id", "device_id"}` |
| `POST /v1/voice` | A WAV file. Headers `X-Session-Id` and `X-Device-Id`. |
| `POST /v1/session/reset` | Forget one conversation. |
| `WS /ws` | Same chat as the HTTP path, for experiments. The watch uses HTTP. |

## Roadmap

These are not in this version. Each one has a marked place in the code so the next phase does not have to invent a structure.

**Bluetooth when Wi-Fi is down.** The watch talks to the laptop over Wi-Fi today. `firmware/src/brain_link.h` is the seam. A later `BleBrainLink` should be used only while Wi-Fi cannot reach the laptop, then the watch should switch back to Wi-Fi by itself. The worker has a TODO at the spot where a failed Wi-Fi send would hand off.

**Tailscale, so the watch can reach the laptop away from home.** `TAILSCALE_ENABLED` in `.env` is read and shown on `/health`. It does not open a tunnel by itself. When you want this: install Tailscale on the laptop, turn the flag on as a reminder, and type the laptop's Tailscale address into the watch setup page (`http://100.x.x.x:8787`). A later phase can make that flag actually start and stop the path. The brain already listens on all of the laptop's addresses (`JARVIS_HOST=0.0.0.0`).

**Spoken replies.** `brain/jarvis_brain/tts/__init__.py` is where audio bytes would be produced. The watch would play them through the ES8311 codec. The pins are already named in `include/pin_config.h` (`PIN_ES8311_DOUT` and `PA` on GPIO 46). `firmware/src/voice.cpp` has a TODO at the playback spot. `TTS_PROVIDER` in `.env` is the switch. Replies today are text only, and the JSON field `speech_available` is false.

**Hermes, or any other agent.** Set `LLM_PROVIDER=hermes` and `HERMES_BASE_URL` to a server that speaks the OpenAI chat API. The class is `brain/jarvis_brain/llm/hermes.py`. If Hermes speaks a different protocol, replace `complete()` in that file. The watch does not change. A local model is the same idea with `LLM_PROVIDER=local` and a `/v1` address.

**Motion.** The QMI8658 IMU is on the same I2C bus (address 0x6B). Waveshare's `04_LVGL_QMI8658_ui` demo shows how to read it. This phase does not.

## For the next person who edits the code

Pins are not guesses. `firmware/include/pin_config.h` is Waveshare's `board_config/pin_config.h`. The display object is constructed the same way as their `01_HelloWorld` sketch, including the column offset of 6. Touch uses SensorLib's CST92xx driver the way their LVGL demo does (`setMirrorXY(true, true)`, max 466×466). The microphone's registers are their `06_ES7210` driver, copied into `firmware/lib/es7210`. The I2S bus is opened with `ESP_I2S.h`, which is what their `07_ES8311` sketch uses, because current Arduino-ESP32 no longer exposes the older `driver/i2s.h` as a normal include. Sample rate is still 16 kHz, 16-bit, mono.

Flash and PSRAM follow their ESP-IDF example `sdkconfig.defaults`: quad flash (`QIO`), 32 MB flash, octal PSRAM. That is the `qio_opi` memory type in `firmware/boards/waveshare_esp32_s3_touch_amoled_175c.json`. The partition table keeps the 24 KB `nvs` and the `phy_init` partition their example uses, which Wi-Fi setup needs. The program itself sits in a 4 MB slot, which is why PlatformIO's size line says 4 MB. The chip is still the 32 MB part.

Libraries are pinned in `firmware/platformio.ini`. Waveshare ships GFX 1.6.4. That release does not compile on Arduino-ESP32 3.3.6 or newer. This project uses GFX 1.6.5, which is the same library with that build fix. The CO5300 panel setup is unchanged.
