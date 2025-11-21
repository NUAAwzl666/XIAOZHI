#!/usr/bin/env python3
"""
测试DeepSeek API的Python脚本
用于验证API密钥和网络连接是否正常
"""

import requests
import json

# DeepSeek API配置
API_KEY = "sk-5f4807087b154707b6586f6eae1fc8c0"  # 使用正确的API密钥
API_URL = "https://api.deepseek.com/v1/chat/completions"

def test_deepseek_api():
    """测试DeepSeek API连接"""
    print("🔥 测试DeepSeek API连接...")
    print(f"API URL: {API_URL}")
    print(f"API Key: {API_KEY[:20]}...")
    
    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {API_KEY}",
        "User-Agent": "Python-Test/1.0"
    }
    
    # 构建请求体 - 与ESP32完全相同的格式
    data = {
        "model": "deepseek-chat",
        "messages": [
            {"role": "system", "content": "You are a helpful assistant"},
            {"role": "user", "content": "你好"}
        ],
        "max_tokens": 50,
        "stream": False
    }
    
    print(f"\n📤 发送请求:")
    print(f"Headers: {json.dumps(headers, indent=2, ensure_ascii=False)}")
    print(f"Data: {json.dumps(data, indent=2, ensure_ascii=False)}")
    
    try:
        response = requests.post(API_URL, headers=headers, json=data, timeout=30)
        
        print(f"\n📥 响应信息:")
        print(f"Status Code: {response.status_code}")
        print(f"Headers: {dict(response.headers)}")
        print(f"Content Length: {len(response.content)} bytes")
        
        if response.status_code == 200:
            print(f"\n✅ 请求成功!")
            response_data = response.json()
            print(f"Response: {json.dumps(response_data, indent=2, ensure_ascii=False)}")
            
            if "choices" in response_data and len(response_data["choices"]) > 0:
                message = response_data["choices"][0]["message"]["content"]
                print(f"\n🤖 AI回复: {message}")
            else:
                print("\n⚠️  响应格式异常")
        else:
            print(f"\n❌ 请求失败")
            print(f"Error: {response.text}")
            
    except requests.exceptions.Timeout:
        print("\n⏰ 请求超时")
    except requests.exceptions.ConnectionError:
        print("\n🌐 连接错误")
    except requests.exceptions.RequestException as e:
        print(f"\n💥 请求异常: {e}")
    except json.JSONDecodeError as e:
        print(f"\n📄 JSON解析错误: {e}")
    except Exception as e:
        print(f"\n🚨 未知错误: {e}")

def test_simple_http():
    """测试简单HTTP连接"""
    print("\n\n🔥 测试基础HTTP连接...")
    
    try:
        response = requests.get("http://httpbin.org/get", timeout=10)
        print(f"HTTP Status: {response.status_code}")
        print(f"Content Length: {len(response.content)} bytes")
        
        if response.status_code == 200:
            print("✅ HTTP连接正常")
            data = response.json()
            print(f"Response Preview: {json.dumps(data, indent=2)[:200]}...")
        else:
            print("❌ HTTP连接失败")
            
    except Exception as e:
        print(f"💥 HTTP测试失败: {e}")

if __name__ == "__main__":
    print("=" * 60)
    print("DeepSeek API 测试工具")
    print("=" * 60)
    
    # 测试基础HTTP连接
    test_simple_http()
    
    # 测试DeepSeek API
    test_deepseek_api()
    
    print("\n" + "=" * 60)
    print("测试完成")
    print("=" * 60)