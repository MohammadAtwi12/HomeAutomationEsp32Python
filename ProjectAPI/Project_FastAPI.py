from SpeechRec import SpeechRecognition
import cv2
from fastapi import FastAPI
from starlette.responses import StreamingResponse
from FaceRec import Process
import asyncio
app = FastAPI()

camera = cv2.VideoCapture(0)

async def gen_frames():
    while True:
        success, frame = camera.read()
        if not success:
            break
        else:
            processed_frame = Process(frame ,True)
            ret, buffer = cv2.imencode('.jpg', processed_frame)
            frame_bytes = buffer.tobytes()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
        await asyncio.sleep(0.1)


@app.get("/speak")
async def speak():
    result =  SpeechRecognition()
    return result
        
@app.get('/video_feed')
async def video_feed():
    return StreamingResponse(gen_frames(), media_type='multipart/x-mixed-replace; boundary=frame')

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=5000)

# Command to run the API : uvicorn Arduino_AI_Test.ProjectAPI.Project_FastAPI:app --reload --host 0.0.0.0