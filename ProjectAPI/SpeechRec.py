import speech_recognition as sr
import pygame

def play_sound(sound_file):
    pygame.mixer.init()
    pygame.mixer.music.load(sound_file)
    pygame.mixer.music.play()


def recognize_speech(recognizer, microphone, wake_word):
    with microphone as source:
        recognizer.adjust_for_ambient_noise(source)
        print("Listening for wake-up word...")
        while True:
            audio = recognizer.listen(source)
            try:
                spoken_text = recognizer.recognize_google(audio)
                if spoken_text.lower() == wake_word:
                    play_sound("Mini_Project_AI\ProjectAPI\on.wav")
                    print("Please speak...")
                    audio = recognizer.listen(source)
                    play_sound("Mini_Project_AI\ProjectAPI\off.wav")
                    print("Processing...")
                    return recognizer.recognize_google(audio)
            except sr.UnknownValueError:
                continue
            except sr.RequestError:
                print("API unavailable.")
                return False

def SpeechRecognition():
    recognizer = sr.Recognizer()
    microphone = sr.Microphone()

    while True:
        text = recognize_speech(recognizer, microphone, "assistant")
        return text