#!/usr/bin/env python3
"""
DeepSeek API 代理服务器
将HTTPS请求转换为HTTP，供ESP32使用
"""

from flask import Flask, request, jsonify
import requests
import json
import logging

app = Flask(__name__)
logging.basicConfig(level=logging.INFO)

# DeepSeek API配置
DEEPSEEK_API_KEY = "sk-5f4807087b154707b6586f6eae1fc8c0"
DEEPSEEK_API_URL = "https://api.deepseek.com/v1/chat/completions"

@app.route('/chat', methods=['POST'])
def chat_proxy():
    """代理DeepSeek聊天API"""
    try:
        # 获取ESP32发送的数据
        esp_data = request.get_json()
        print(f"收到ESP32请求: {json.dumps(esp_data, indent=2, ensure_ascii=False)}")
        
        # 准备发送给DeepSeek的请求
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {DEEPSEEK_API_KEY}",
            "User-Agent": "ESP32-Proxy/1.0"
        }
        
        # 发送请求到DeepSeek
        response = requests.post(DEEPSEEK_API_URL, headers=headers, json=esp_data, timeout=30)
        
        print(f"DeepSeek响应状态: {response.status_code}")
        print(f"DeepSeek响应长度: {len(response.content)} bytes")
        
        if response.status_code == 200:
            result = response.json()
            print(f"DeepSeek响应: {json.dumps(result, indent=2, ensure_ascii=False)}")
            return jsonify(result)
        else:
            error_msg = f"DeepSeek API错误: {response.status_code} - {response.text}"
            print(error_msg)
            return jsonify({"error": error_msg}), response.status_code
            
    except requests.exceptions.Timeout:
        error_msg = "DeepSeek API请求超时"
        print(error_msg)
        return jsonify({"error": error_msg}), 408
    except requests.exceptions.RequestException as e:
        error_msg = f"DeepSeek API请求失败: {str(e)}"
        print(error_msg)
        return jsonify({"error": error_msg}), 500
    except Exception as e:
        error_msg = f"服务器内部错误: {str(e)}"
        print(error_msg)
        return jsonify({"error": error_msg}), 500

@app.route('/health', methods=['GET'])
def health_check():
    """健康检查"""
    return jsonify({"status": "ok", "service": "DeepSeek API Proxy"})

@app.route('/test', methods=['GET'])
def test_api():
    """测试DeepSeek API连接"""
    test_data = {
        "model": "deepseek-chat",
        "messages": [
            {"role": "system", "content": "You are a helpful assistant"},
            {"role": "user", "content": "Hello"}
        ],
        "max_tokens": 50,
        "stream": False
    }
    
    try:
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {DEEPSEEK_API_KEY}",
            "User-Agent": "ESP32-Proxy-Test/1.0"
        }
        
        response = requests.post(DEEPSEEK_API_URL, headers=headers, json=test_data, timeout=30)
        
        if response.status_code == 200:
            return jsonify({
                "status": "success",
                "message": "DeepSeek API连接正常",
                "response": response.json()
            })
        else:
            return jsonify({
                "status": "error", 
                "message": f"DeepSeek API错误: {response.status_code}",
                "error": response.text
            }), response.status_code
            
    except Exception as e:
        return jsonify({
            "status": "error",
            "message": f"API测试失败: {str(e)}"
        }), 500

if __name__ == '__main__':
    print("🚀 启动DeepSeek API代理服务器...")
    print(f"🔑 API Key: {DEEPSEEK_API_KEY[:20]}...")
    print("📝 可用端点:")
    print("   POST /chat - 聊天API代理")
    print("   GET /health - 健康检查")
    print("   GET /test - API连接测试")
    print("🌐 服务器地址: http://192.168.45.100:5000")
    print("=" * 50)
    
    # 在所有接口上监听，ESP32可以访问
    app.run(host='0.0.0.0', port=5000, debug=True)