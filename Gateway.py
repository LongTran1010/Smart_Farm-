print("Hello Core IOT")
import paho.mqtt.client as mqttclient
import time
import json
import requests


# Cấu hình MQTT Local (Giao tiếp với ESP32)
LOCAL_MQTT_HOST = "192.168.90.50"
LOCAL_MQTT_PORT = 1883
LOCAL_MQTT_TOPICS_SUB = [
    "home/sensors/dht",
    "home/sensors/light",
    "home/sensors/sm",
    "home/sensors/leaf"]
LOCAL_MQTT_TOPIC_PUB = "home/actuators/mqtt"
LOCAL_FW_VERSION_TOPIC = "home/device/fw_version"

#MQTT Core IOT
BROKER_ADDRESS = "app.coreiot.io"
ACCESS_TOKEN = "Longteo@123"
ACCESS_USERNAME = "longthangtran456"
COREIOT_SUB_TOPIC = "v1/devices/me/rpc/request/+" #topic nhận lệnh từ core iot

#Firmware
current_fw_version = "1.0.1"
FIRMWARE_JSON_URL = "https://drive.google.com/uc?export=download&id=1El0QW66C0PpML7bTCTsdEnb6GwaV_g_R"
CHECK_INTERVAL = 600
last_check_time = 0

#Lấy phiên bản firmware mới nhất từ gg drive
def get_latest_firmware():
    try:
        response = requests.get(FIRMWARE_JSON_URL, timeout=10)
        response.raise_for_status()

        data = response.json()
        return data.get("fw_version"), data.get("fw_url")

    except requests.RequestException as e:
        print(f"Error fetching firmware JSON: {e}")
        return None, None
    except json.JSONDecodeError:
        print("Error decoding JSON response")
        return None, None

#Gửi OTA request tới ESP32
def send_ota_update():
    global last_check_time
    latest_fw_version, latest_fw_url = get_latest_firmware()
    if not latest_fw_version or not latest_fw_url:
        return

    if latest_fw_version and latest_fw_url and current_fw_version != latest_fw_version:
        ota_payload = json.dumps({"fw_version": latest_fw_version, "fw_url": latest_fw_url})
        print(f"Sending OTA update: {ota_payload}")
        esp_client.publish(LOCAL_MQTT_TOPIC_PUB, ota_payload)
    last_check_time = time.time()

#Nhận firmware version từ ESP32
def on_fw_version_message(client, userdata, message):
    global current_fw_version
    try:
        data = json.loads(message.payload.decode("utf-8"))
        if "fw_version" in data:
            current_fw_version = data["fw_version"]
            print(f"ESP32 firmware version: {current_fw_version}")
            send_ota_update()
    except json.JSONDecodeError:
        print("Error: Received invalid JSON from ESP32")

#Kết nối CoreIOT
def connected(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully!!")
        tb_client.subscribe(COREIOT_SUB_TOPIC)
    else:
        print("Connection failed, rc =", rc)

def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed...")

# Hàm xử lý khi nhận lệnh từ CoreIOT
def on_tb_message(client, userdata, message):
    try:
        data = json.loads(message.payload.decode("utf-8"))
        print(f"Received RPC from Core IOT: {data}")
        raw_params = data.get("params")
        if isinstance(raw_params, dict) and "params" in raw_params:
            pump_val = raw_params["params"]
        else:
            pump_val = raw_params
        #request_id = message.topic.split('/')[-1] #Lấy request ID từ topic
        #Đèn LED
        if 'method' in data and data['method'] == "setValueLED":
            led_state = data['params']
            esp_payload = json.dumps({"led": led_state})
            # Gửi lệnh LED xuống ESP32
            esp_client.publish(LOCAL_MQTT_TOPIC_PUB, esp_payload)
            print(f"Sent to ESP32: {esp_payload}")
        #Scheduled ON
        elif 'method' in data and data['method'] == "setValuePUMP1":
            esp_client.publish(LOCAL_MQTT_TOPIC_PUB, json.dumps({"pump": True}))
            print(f"Sent to ESP32 scheduled pump ON")
        #Scheduled OFF
        elif 'method' in data and data['method'] == "setValuePUMP2":
            esp_client.publish(LOCAL_MQTT_TOPIC_PUB, json.dumps({"pump": False}))
            print(f"Sent to ESP32 scheduled pump OFF")
        #Thủ công
        elif 'method' in data and data['method'] == "setValuePump":
            pump_state = data.get("params", False)
            esp_client.publish(LOCAL_MQTT_TOPIC_PUB, json.dumps({"pump1": pump_state}))
            print(f"Sent to ESP32 scheduled pump :{json.dumps({"pump1": pump_state})}")
        else:
            print("Unknown RPC method.")
            return
    except json.JSONDecodeError:
        print("Error: Received invalid JSON from Core IOT")
    except Exception as e:
        print("Error processing message:", e)

#Hàm xử lý khi nhận dữ liệu từ ESP32
def recv_esp_message(client, userdata, message):
    try:
        json_str = message.payload.decode("utf-8")
        data = json.loads(json_str)
        print(f"Received from ESP32: {data}")

        telemetry = {}
        if "temperature" in data:
            telemetry["temperature"] = data["temperature"]
        if "humidity" in data:
            telemetry["humidity"] = data["humidity"]
        if "soil_moisture" in data:
            telemetry["soil_moisture"] = data["soil_moisture"]
        if "light_status" in data:
            telemetry["light_status"] = data["light_status"]

        #Dành cho gg teachable machine, xử lí nhận dữ liệu từ AI gateway
        if "leaf_label" in data:
            telemetry["leaf_label"] = data["leaf_label"]
        if "confidence" in data:
            conf = round(data["confidence"] * 100, 2)
            telemetry["leaf_confidence"] = conf

        if telemetry and tb_client.is_connected():
            tb_client.publish("v1/devices/me/telemetry", json.dumps(telemetry))
            print(f"Sent to CoreIOT: {telemetry}")
        else:
            print("No valid telemetry fields found in received data.")

    except json.JSONDecodeError:
        print("Error: Received invalid JSON from ESP32")
    except Exception as e:
        print("Error processing message:", e)

#Khởi tạo và run MQTT clients
esp_client = mqttclient.Client(client_id="", protocol=mqttclient.MQTTv311)# dùng MQTT 3.1.1
esp_client.on_message = recv_esp_message
esp_client.on_connect = lambda c, u, f, rc: (
    [esp_client.subscribe(t) or print(f"Subcribed {t}") for t in LOCAL_MQTT_TOPICS_SUB],
    esp_client.subscribe(LOCAL_FW_VERSION_TOPIC),
    esp_client.message_callback_add(LOCAL_FW_VERSION_TOPIC, on_fw_version_message)
)
esp_client.connect(LOCAL_MQTT_HOST, LOCAL_MQTT_PORT, 60)
esp_client.loop_start()

tb_client = mqttclient.Client(client_id="IOT_DEVICE_2", protocol=mqttclient.MQTTv311)
tb_client.username_pw_set(ACCESS_USERNAME, ACCESS_TOKEN)
tb_client.on_connect = connected
tb_client.on_subscribe = subscribed
tb_client.on_message = on_tb_message
tb_client.connect(BROKER_ADDRESS, 1883, 60)
tb_client.loop_start()

print("===Gateway started===")

#Kiểm tra OTA
while True:
    try:
        if not tb_client.is_connected():
            print("Reconnecting to CoreIOT...")
            tb_client.connect(BROKER_ADDRESS, 1883, 60)

        if time.time() - last_check_time > CHECK_INTERVAL:
            print("Checking for firmware update...")
            send_ota_update()
        time.sleep(1)

    except Exception as e:
        print("Error in main loop:", e)
        time.sleep(5)