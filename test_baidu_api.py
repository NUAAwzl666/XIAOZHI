#!/usr/bin/env python3
"""
测试百度语音API的访问令牌获取
"""

import requests
import json

def test_baidu_token():
    """测试百度语音API访问令牌获取"""
    
    # 从config.h中的密钥
    app_id = "120197558"
    api_key = "iw0uVTjdNE3EHj5I0ZliSj8Z"
    secret_key = "M34aLHQUH2mkK3MDLQ540jfpiVUJpz6n"
    
    token_url = "https://aip.baidubce.com/oauth/2.0/token"
    
    data = {
        "grant_type": "client_credentials",
        "client_id": api_key,
        "client_secret": secret_key
    }
    
    try:
        print("=" * 50)
        print("测试百度语音API访问令牌获取")
        print("=" * 50)
        print(f"APP_ID: {app_id}")
        print(f"API_KEY: {api_key[:8]}...")
        print(f"SECRET_KEY: {secret_key[:8]}...")
        print(f"Token URL: {token_url}")
        print()
        
        print("发送请求...")
        response = requests.post(token_url, data=data, timeout=15)
        
        print(f"HTTP状态码: {response.status_code}")
        print(f"响应长度: {len(response.content)} bytes")
        
        if response.status_code == 200:
            result = response.json()
            print(f"响应内容: {json.dumps(result, indent=2, ensure_ascii=False)}")
            
            if "access_token" in result:
                token = result["access_token"]
                expires_in = result.get("expires_in", 0)
                print(f"\n✓ 成功获取访问令牌")
                print(f"令牌: {token[:20]}...")
                print(f"有效期: {expires_in} 秒")
                return True
            else:
                print(f"\n✗ 响应中缺少access_token字段")
                return False
        else:
            print(f"✗ HTTP错误: {response.status_code}")
            print(f"错误响应: {response.text}")
            return False
            
    except Exception as e:
        print(f"✗ 请求失败: {e}")
        return False

if __name__ == "__main__":
    success = test_baidu_token()
    print("\n" + "=" * 50)
    if success:
        print("✓ 百度API密钥验证成功")
    else:
        print("✗ 百度API密钥验证失败")