import serial
import time
import  requests

port = 'COM7'
baud_rate = 115200
WEB_APP_URL =" "
try:
    with serial.Serial(port, baud_rate, timeout=1) as ser:
        print(f"Connected to {port} at {baud_rate} baud.")
        time.sleep(1)  # Wait for ESP32 to reset and start

        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()

                if line:	
                    print(f"> {line}")
                    requests.post(WEB_APP_URL, data=line)
except serial.SerialException as e:
    print(f"Error: {e}")
