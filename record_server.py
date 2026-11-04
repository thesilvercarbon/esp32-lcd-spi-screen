import queue, socket, threading, time, wave
import numpy as np
from faster_whisper import WhisperModel

PORT, RATE = 5000, 16000
MODEL, LANG = "small", None        # LANG = "en" to force English
SKIP = RATE * 50 // 1000           # drop the first 50 ms (mic start-up spike)


def load_model():
    print(f"loading model '{MODEL}' ...")
    return WhisperModel(MODEL, device="cpu", compute_type="int8")


def clean(pcm: bytes) -> np.ndarray:
    x = np.frombuffer(pcm[: len(pcm) // 2 * 2], dtype="<i2").astype(np.float32)
    x = x[SKIP:]
    if len(x) == 0:
        return x
    x -= x.mean()                                   # remove DC offset
    peak = np.abs(x).max()
    if peak > 0:
        x *= 0.7 * 32768 / peak                     # about -3 dBFS
    return x / 32768.0                              # Whisper wants floats in [-1, 1]


def transcribe(model, audio: np.ndarray) -> str:
    if len(audio) == 0:
        return ""
    segments, _ = model.transcribe(audio, language=LANG, beam_size=1, vad_filter=True)
    return " ".join(s.text.strip() for s in segments).strip()


def worker(model, q):
    while True:
        name, pcm = q.get()
        text = transcribe(model, clean(pcm))
        print(f"{name}: {text or '(no speech detected)'}")


def main():
    model = load_model()
    q = queue.Queue()
    threading.Thread(target=worker, args=(model, q), daemon=True).start()

    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", PORT))
    srv.listen(1)
    print(f"listening on port {PORT}")

    while True:
        conn, _ = srv.accept()
        data = bytearray()
        with conn:
            while chunk := conn.recv(4096):
                data += chunk
        name = time.strftime("rec_%H%M%S.wav")
        with wave.open(name, "wb") as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
            w.writeframes(bytes(data))
        print(f"{name}: saved {len(data)//2} samples, {len(data)/2/RATE:.2f} s")
        q.put((name, bytes(data)))


main()
