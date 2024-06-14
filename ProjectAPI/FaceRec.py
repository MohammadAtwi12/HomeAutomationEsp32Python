import cv2
import numpy as np
import face_recognition
import firebase_admin
from firebase_admin import credentials, db, storage, messaging


def InitFireBase():
    cred = credentials.Certificate("Mini_Project_AI/arduino-iot-ae62c-firebase-adminsdk-swv1c-8449043da2.json")
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://arduino-iot-ae62c-default-rtdb.firebaseio.com/',
        'storageBucket': 'arduino-iot-ae62c.appspot.com'
    })

def send_fcm_message(token, title, body):
    message = messaging.Message(
        notification=messaging.Notification(
            title=title,
            body=body,
        ),
        token=token,
    )

    # Send a message to the device corresponding to the provided registration token.
    response = messaging.send(message)
    # Response is a message ID string.
    print('Successfully sent message:', response)

'''def load_known_faces():
    known_face_encodings = []
    known_face_names = []
    for filename in os.listdir('Face_Recognition/faces'):
        image = face_recognition.load_image_file(f'Face_Recognition/faces/{filename}')
        face_encoding = face_recognition.face_encodings(image)[0]
        known_face_encodings.append(face_encoding)
        known_face_names.append(filename.split('.')[0])
    return known_face_encodings, known_face_names'''

def load_known_faces():
    known_face_encodings = []
    known_face_names = []

    # Initialize Firebase Storage client
    bucket = storage.bucket()

    # List objects in the "KnownFaces" folder
    blobs = bucket.list_blobs(prefix='KnownFaces/')
    for blob in blobs:
        if blob.name.endswith('/'):
            continue

        try:
            image_bytes = blob.download_as_bytes()

            image = cv2.imdecode(np.frombuffer(image_bytes, np.uint8), -1)

            if image is None:
                print(f"Error: Unable to decode image '{blob.name}'")
                continue

            face_encoding = face_recognition.face_encodings(image)[0]

            # Extract name from blob name
            name = blob.name.split('/')[1].split('.')[0]

            # Append face encoding and name to lists
            known_face_encodings.append(face_encoding)
            known_face_names.append(name)
        except Exception as e:
            print(f"Error processing image '{blob.name}': {e}")

    return known_face_encodings, known_face_names


def reload_listener(event):
    if event.data == True:
        print("Reload key changed to true, reloading known faces.")
        global known_face_encodings, known_face_names
        known_face_encodings, known_face_names = load_known_faces()
        ref.set(False)  # Reset the reload key to false after reloading

def fireDetection(event):
    if event.data == True:
        send_fcm_message(registration_token,"Warning","Fire in the House")
        ref2.set(False)

def rainDetection(event):
    if event.data == True:
        send_fcm_message(registration_token,"Warning","It is Raining, Close the Windows")
        ref3.set(False)

InitFireBase()
ref = db.reference('reload')
ref2 = db.reference('Fire')
ref3 = db.reference('Rain')
ref.listen(reload_listener)
ref2.listen(fireDetection)
ref3.listen(rainDetection)

registration_token = 'e4PG8PKPTfagiLP1PsNcW0:APA91bFlQZYRyLOLW2vQpvRxd-zjJHceLtAmKBucnavpFfpv5VrtCwneXetGSxbfEtTliH6maf5Vp77Dxw5PEVg163si7Nd9lcyd1MRiq9C_FD5kV3FdKL7OHt2wAVytCtc5Beuqdtos'

known_face_encodings, known_face_names = load_known_faces()

messageBody = ''
prevMesggageBody = ''

def Process(frame , process_this_frame):
    # Initialize some variables
    face_locations = []
    face_encodings = []
    face_names = []
    global messageBody
    global prevMesggageBody

    # Only process every other frame of video to save time
    if process_this_frame:
        # Resize frame of video to 1/4 size for faster face recognition processing
        small_frame = cv2.resize(frame, (0, 0), fx=1, fy=1)

        # Convert the image from BGR color (which OpenCV uses) to RGB color (which face_recognition uses)
        rgb_small_frame = small_frame[:, :, ::-1]

        # Find all the faces and face encodings in the current frame of video
        face_locations = face_recognition.face_locations(rgb_small_frame)
        face_encodings = face_recognition.face_encodings(rgb_small_frame, face_locations)

        face_names = []
        for face_encoding in face_encodings:
            # See if the face is a match for the known face(s)
            matches = face_recognition.compare_faces(known_face_encodings, face_encoding)
            name = "Unknown"

            # Or instead, use the known face with the smallest distance to the new face
            face_distances = face_recognition.face_distance(known_face_encodings, face_encoding)
            best_match_index = np.argmin(face_distances)
            if matches[best_match_index]:
                name = known_face_names[best_match_index]

            face_names.append(name)
        if face_names != [] :
            knames = ''
            unames = ''
            for name in face_names:
                if name != "Unknown":
                    knames += (name+', ')
                else:
                    unames = "Unknown People"
            messageBody = knames+unames+' at the Door'
            if (messageBody != prevMesggageBody):
                send_fcm_message(registration_token, 'Door Camera', messageBody)
                prevMesggageBody = messageBody
        else:
            messageBody = ''


    '''for (top, right, bottom, left), name in zip(face_locations, face_names):
        # Scale back up face locations since the frame we detected in was scaled to 1/4 size
        top *= 1
        right *= 1
        bottom *= 1
        left *= 1

        # Draw a box around the face
        cv2.rectangle(frame, (left, top), (right, bottom), (0, 0, 255), 2)

        # Draw a label with a name below the face  
        cv2.rectangle(frame, (left, bottom - 35), (right, bottom), (0, 0, 255), cv2.FILLED)
        font = cv2.FONT_HERSHEY_DUPLEX
        cv2.putText(frame, name, (left + 6, bottom - 6), font, 1.0, (255, 255, 255), 1)'''
    
    return frame


