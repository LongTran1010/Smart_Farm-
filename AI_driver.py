print("Hello Core IOT")
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

import time
import json
import base64
import threading

import cv2
import numpy as np
import paho.mqtt.client as mqttclient

from keras.models import load_model
from depthwise_wrapper import DepthwiseConv2D

np.set_printoptions(suppress=True)

# Load model và labels
model = load_model("keras_model.h5", compile=False, custom_objects={"DepthwiseConv2D": DepthwiseConv2D})
with open("labels.txt", "r", encoding="utf-8") as f:
    class_names = f.readlines()

# Mở cam (0 hay 1 tùy vào cam mặc định trên máy)
camera = cv2.VideoCapture(0)
# Biến để lưu kết quả
r_class = "NONE"
r_data = 0

# Cấu hình MQTT Local
LOCAL_MQTT_HOST = "192.168.90.50"
LOCAL_MQTT_PORT = 1883
LOCAL_MQTT_TOPIC = "home/sensors/leaf"
esp_client = mqttclient.Client("AIGateway", protocol=mqttclient.MQTTv311)
esp_client.connect(LOCAL_MQTT_HOST, LOCAL_MQTT_PORT, 60)
esp_client.loop_start()
last_publish_time = 0.0
PUBLISH_INTERVAL = 5.0  # giãn cách 5 giây

#Hàm Execute chụp 1 khung hình, inference, trả về base64 + tên class
def AI_Execute():
    global r_class, r_data, last_publish_time

    ret, image = camera.read()
    if not ret:
        print("[AI] Camera failed to capture image.")
        return "", "NONE"

    image = cv2.resize(image, (224, 224), interpolation = cv2.INTER_AREA)
    data = image.copy() #giữ ảnh gốc

    image = np.asarray(image, dtype=np.float32).reshape(1, 224, 224, 3)
    image = (image / 127.5) - 1 #normalize về [-1,1]

    prediction = model.predict(image)
    index = np.argmax(prediction)
    confidence_score = prediction[0][index]
    line = class_names[index].strip()
    class_name = line.split(" ", 1)[1]
    if confidence_score > 0.8:
        r_class = class_name
    # Gửi kết quả
    else:
        r_class = "Uncertain"

    print("[AI] Class:", r_class, "Confidence:", f"{confidence_score*100:.2f}%")

    #Ghi tên class lên ảnh
    cv2.putText(data, class_name, (90, 40),
                cv2.FONT_HERSHEY_DUPLEX, 0.5, (0, 0, 255), 1)
    #cv2.imshow("Webcam Image", data)
    # Encode ảnh
    ret2, img_buffer = cv2.imencode('.jpg', data, [cv2.IMWRITE_JPEG_QUALITY, 40])
    img_base64 = base64.b64encode(img_buffer).decode('utf-8')
    r_data = img_base64

    now = time.time()
    # Nếu đã quá 5 giây kể từ lần publish trước, mới gửi payload
    if now - last_publish_time >= PUBLISH_INTERVAL:
        payload = {
            "leaf_label": r_class,
            "confidence": float(confidence_score),
            "timestamp": int(now * 1000)
        }
        try:
            esp_client.publish(LOCAL_MQTT_TOPIC, json.dumps(payload))
            last_publish_time = now  # cập nhật lại thời điểm vừa publish
            print(f"[MQTT] Published at {time.strftime('%H:%M:%S', time.localtime(now))}")
        except Exception as e:
            print(f"[MQTT] Publish failed → {e}")
    #cv2.waitKey(10)
    return img_base64, r_class

#Hàm cập nhật r_data và r_class
def AI_Start():
    global r_class, r_data
    while True:
        r_data, r_class = AI_Execute()
        time.sleep(0.1)



# Chạy AI ở luồng riêng để không chặn main loop
ai_thread = threading.Thread(target=AI_Start)
ai_thread.daemon = True
ai_thread.start()

#Hàm stop camera và dọn dẹp
def AI_Stop():
    camera.release()
    cv2.destroyAllWindows()

#Hàm lấy dữ liệu mới nhất
def AI_Get():
    global r_data, r_class
    return r_data, r_class

#Main thread
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Stopping AI...")
    AI_Stop()
    esp_client.loop_stop()
    esp_client.disconnect()
    print("Exit.")