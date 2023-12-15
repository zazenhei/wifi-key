#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

//家庭内WiFi設定(2G)
#define WIFI_SSID "IexWarehouse2G"
#define WIFI_PASSWORD "WarnUserActivityIsLoggedByUTM"
//鍵のパスワード
#define KEY_PASSWORD "IexTest"
//解錠してから自動で施錠するまでのミリ秒数
#define AUTO_LOCK_MS 10000
//HTTPサーバーのポート
#define HTTP_SERVER_PORT 3080
//モーター制御のピン
#define SERVO_CONTROL_PIN 3
//プライベートIPアドレス設定
static const IPAddress g_wifi_ip_address(192, 168, 7, 55);
static const IPAddress g_wifi_subnet(255, 255, 255, 0);
static const IPAddress g_wifi_gateway(192, 168, 7, 7);
static const IPAddress g_primary_dns(192, 168, 7, 7);
static const IPAddress g_secondary_dns(192, 168, 7, 7);

//LEDデバッグ
//#define LED_DEBUG

static void page_handle_root(void);
static void page_handle_input(void);
static void page_handle_not_found(void);
void setup(void);
void loop(void);

static ESP8266WiFiMulti g_wifi_multi;
static ESP8266WebServer g_http_server(HTTP_SERVER_PORT);
static unsigned long g_key_open_time = 0;

#ifndef LED_DEBUG
    static Servo g_servo;
#endif

static void page_handle_root(void)
{
	//パスワード入力ページ表示
	g_http_server.send(200, "text/html", "<html><body><form method=\"get\" action=\"/input\"><p>Input password<br><input type=\"password\" name=\"p\"><input type=\"submit\" value=\"OK\"></p></form></body></html>");
}

static void page_handle_input(void)
{
	//認証
	for(int i = 0; g_http_server.args() > i; i++)
	{
		const String name = g_http_server.argName(i);
		const String value = g_http_server.arg(i);

		if(String("p") != name || String(KEY_PASSWORD) != value)
		{
			continue;
		}

		//解錠
		g_key_open_time = millis();
		break;
	}

	if((g_key_open_time + AUTO_LOCK_MS) < millis())
	{
		g_http_server.send(200, "text/plain", "Failed.");
	}
	else
	{
		g_http_server.send(200, "text/plain", "OK.");
	}
}

static void page_handle_not_found(void)
{
	g_http_server.send(404, "text/plain", "404: Not found");
}

void setup(void)
{
	//制御ピン初期化
    #ifdef LED_DEBUG
        pinMode(SERVO_CONTROL_PIN, OUTPUT);
        digitalWrite(SERVO_CONTROL_PIN, LOW);
    #else
        g_servo.attach(SERVO_CONTROL_PIN);
        g_servo.write(0);
    #endif

	//WiFi接続
	WiFi.mode(WIFI_STA);
	g_wifi_multi.addAP(WIFI_SSID, WIFI_PASSWORD);
	WiFi.config(g_wifi_ip_address, g_wifi_gateway, g_wifi_subnet, g_primary_dns, g_secondary_dns);
	while(g_wifi_multi.run() != WL_CONNECTED)
	{
		delay(250);
	}

	//HTTPサーバー設定
	g_http_server.on("/", page_handle_root);
	g_http_server.on("/input", page_handle_input);
	g_http_server.onNotFound(page_handle_not_found);

	//HTTPサーバー開始
	g_http_server.begin();
}

void loop(void)
{
	//HTTPサーバー処理
	g_http_server.handleClient();

	//サーボ制御
	if((g_key_open_time + AUTO_LOCK_MS) > millis())
	{
		//解錠
        #ifdef LED_DEBUG
            digitalWrite(SERVO_CONTROL_PIN, HIGH);
        #else
            g_servo.write(180);
        #endif
	}
	else
	{
		//施錠
		#ifdef LED_DEBUG
            digitalWrite(SERVO_CONTROL_PIN, LOW);
        #else
            g_servo.write(0);
        #endif
	}
}
