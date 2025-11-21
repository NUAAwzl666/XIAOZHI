import serial
import time
import sys

def test_voice_recognition():
    try:
        # 连接串口
        ser = serial.Serial('COM5', 115200, timeout=1)
        print("已连接到 ESP32-S3")
        
        # 等待设备初始化
        print("等待设备初始化...")
        time.sleep(3)
        
        # 清空缓冲区
        ser.flushInput()
        ser.flushOutput()
        
        # 发送测试命令
        print("发送测试命令...")
        
        # 测试按钮状态
        ser.write(b'button\n')
        time.sleep(0.5)
        
        # 测试系统状态
        ser.write(b'status\n')
        time.sleep(0.5)
        
        # 读取响应
        print("\n=== ESP32-S3 响应 ===")
        start_time = time.time()
        while time.time() - start_time < 5:  # 读取5秒的数据
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8', errors='ignore').strip()
                if data:
                    print(data)
        
        print("\n=== 测试录音功能 ===")
        print("发送录音测试命令...")
        ser.write(b'record\n')
        time.sleep(1)
        
        # 模拟2秒录音
        print("等待录音...")
        time.sleep(2)
        
        # 停止录音
        ser.write(b'stop\n')
        
        # 读取录音结果
        print("\n=== 录音测试结果 ===")
        start_time = time.time()
        while time.time() - start_time < 10:  # 读取10秒的数据
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8', errors='ignore').strip()
                if data:
                    print(data)
                    if "识别结果" in data or "ASR error" in data:
                        break
        
        ser.close()
        print("\n测试完成")
        
    except Exception as e:
        print(f"错误: {e}")

if __name__ == "__main__":
    test_voice_recognition()