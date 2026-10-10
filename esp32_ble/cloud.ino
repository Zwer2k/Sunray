// Cloud WebSocket client (ESP32) to connect robot UART to Sunray cloud
// Similar to linux/httpserver.cpp::processWifiWSClient(), but using ArduinoWebsockets

#include "config.h"

#ifdef USE_CLOUD

#include <Arduino.h>
#include <WiFi.h>
// Use mbedTLS HTTPS client with pinned Let's Encrypt ISRG Root X1 (same as relay)
#include <WiFiClientSecure.h>
#include <time.h>
#include "trust.h"
#include "src/ws/WebSocketClient.h"

// Backoff for reconnect attempts
static Backoff cloudBackoff(1000, 20000, 1.4);
static bool cloudConnected = false;
static unsigned long wsLastRxTime = 0;
static unsigned long wsNextConnectTime = 0;
static WiFiClientSecure cloudTls;
static WebSocketClient* ws = nullptr;

// UART responses are collected by the main loop and forwarded asynchronously.
// This prevents delayed mower responses from being consumed by the generic AT
// parser after the former synchronous one-second wait expired.
static char cloudUartBuffer[1024];
static size_t cloudUartLength = 0;
static unsigned long cloudUartLastByteTime = 0;
static bool cloudCommandPending = false;
static unsigned long cloudCommandTime = 0;

static void cloud_flush_uart() {
  if (cloudUartLength == 0) return;

  cloudUartBuffer[cloudUartLength] = 0;
  CONSOLE.print("UART rx:");
  CONSOLE.write((const uint8_t*)cloudUartBuffer, cloudUartLength);
  if (cloudUartBuffer[cloudUartLength - 1] != '\n') CONSOLE.println();

  if (cloudConnected && ws && ws->connected()) {
    ws->sendText(String(cloudUartBuffer, cloudUartLength));
  }
  cloudUartLength = 0;
  cloudCommandPending = false;
}

void cloud_uart_rx(char ch) {
  if (!cloudConnected) return;

  if (cloudUartLength >= sizeof(cloudUartBuffer) - 1) {
    cloud_flush_uart();
  }
  cloudUartBuffer[cloudUartLength++] = ch;
  cloudUartLastByteTime = millis();
  if (ch == '\n') cloud_flush_uart();
}

bool cloud_is_connected() {
  return cloudConnected;
}

// Helper: build ws/wss URL like Linux version
static String cloud_url() {
  String path;
  path += String("/ws/robot?connect_key=") + String(WS_ROBOT_CONNECT_KEY) + String("&proto=at");
  return path;
}

static void cloud_setup_tls() {
  // Pin Let's Encrypt ISRG Root X1
  cloudTls.setCACert(tls_ca_trust);
  cloudTls.setHandshakeTimeout(15000);
  cloudTls.setTimeout(15000);
  static const char* alpn_protos[] = { "http/1.1", 0 };
  cloudTls.setAlpnProtocols(alpn_protos);
}

void cloud_setup() {
  cloud_setup_tls();
}

static bool cloud_loopConnection() {
  if (cloudConnected) return true;
  if (WiFi.status() != WL_CONNECTED) return false;
  // Wait for system time before TLS
  time_t nowsec = time(nullptr);
  if (nowsec < 1609459200) { // 2021-01-01
    CONSOLE.println("WS: waiting for time sync");
    return false;
  }

  const uint32_t now = millis();
  if (now < wsNextConnectTime) return false;
  wsNextConnectTime = now + cloudBackoff.next();

  String path = cloud_url();
  String masked = path;
  int idx = masked.indexOf("connect_key=");
  //if (idx >= 0) {
  //  int start = idx + 12;
  //  int end = masked.indexOf('&', start);
  //  masked = masked.substring(0, start) + String("***") + (end >= 0 ? masked.substring(end) : String(""));
  //}
  CONSOLE.print("WS: connecting wss://");
  CONSOLE.print(WS_HOST);
  CONSOLE.print(":");
  CONSOLE.print(WS_PORT);
  CONSOLE.print(" path=");
  CONSOLE.println(path);
  IPAddress ip;
  if (WiFi.hostByName(WS_HOST, ip)) {
    CONSOLE.print("WS: resolved ");
    CONSOLE.println(ip);
  } else {
    CONSOLE.println("WS: DNS resolution failed");
  }

  // One-time TLS probe to isolate handshake failures from WS logic
  static bool tlsProbed = false;
  static bool tlsProbeOk = false;
  if (!tlsProbed) {
    tlsProbed = true;
    WiFiClientSecure probe;
    // Reuse pinned CA and settings
    probe.setCACert(tls_ca_trust);
    probe.setHandshakeTimeout(15000);
    probe.setTimeout(15000);
    static const char* alpn_protos[] = { "http/1.1", 0 };
    probe.setAlpnProtocols(alpn_protos);
    CONSOLE.print("TLS probe: connecting to ");
    CONSOLE.print(WS_HOST);
    CONSOLE.print(":");
    CONSOLE.println(WS_PORT);
    esp_task_wdt_reset();
    if (probe.connect(WS_HOST, WS_PORT)) {
      CONSOLE.println("TLS probe: success");
      tlsProbeOk = true;
      probe.stop();
    } else {
      CONSOLE.println("TLS probe: FAILED");
      tlsProbeOk = false;
    }
    esp_task_wdt_reset();
    if (!tlsProbeOk) {
      // Skip WS handshake if TLS itself fails; try again later
      return false;
    }
  }

  if (ws) { delete ws; ws = nullptr; }
  ws = new WebSocketClient(cloudTls, WS_HOST, WS_PORT, path);
  esp_task_wdt_reset();
  if (ws->connect()) {
    CONSOLE.println("WS: connected");
    cloudConnected = true;
    cloudBackoff.reset();
    wsLastRxTime = millis();
    cloudUartLength = 0;
    cloudCommandPending = false;
  } else {
    CONSOLE.print("WS: connect failed (WiFiStatus=");
    CONSOLE.print((int)WiFi.status());
    CONSOLE.print(", localIP=");
    CONSOLE.print(WiFi.localIP());
    CONSOLE.println(")");
  }
  esp_task_wdt_reset();
  return false;
}

void cloud_loop() {
  // If we believe we're connected but the socket isn't, fix state and schedule reconnect
  if (cloudConnected && (!ws || !ws->connected())) {
    CONSOLE.println("WS: transport closed, scheduling reconnect");
    cloudConnected = false;
    cloudUartLength = 0;
    cloudCommandPending = false;
    wsNextConnectTime = millis() + 2000;
    if (ws) { ws->close(); delete ws; ws = nullptr; }
  }

  if (!cloud_loopConnection()) return;

  // Some UART responses do not end in a newline. Flush a partial response
  // after a short quiet period instead of losing it.
  if (cloudUartLength > 0 && (millis() - cloudUartLastByteTime >= 100)) {
    cloud_flush_uart();
  }

  // Do not queue another command while the mower is still answering. Release
  // the slot after a generous timeout so one lost response cannot deadlock the
  // WebSocket receive path.
  if (cloudCommandPending) {
    if (millis() - cloudCommandTime < 3000) return;
    CONSOLE.println("UART response timeout");
    cloudCommandPending = false;
  }

  // Poll one incoming command per loop. The response is picked up at the top
  // of the next main-loop iterations by cloud_uart_rx().
  if (ws && ws->connected()) {
    String msg;
    if (ws->pollText(msg)) {
      wsLastRxTime = millis();
      if (msg.length() == 0) return;
      while (msg.endsWith("\r") || msg.endsWith("\n")) msg.remove(msg.length()-1);
      CONSOLE.print("cloud rx:");
      CONSOLE.println(msg);
      CONSOLE.print("UART tx:");
      CONSOLE.println(msg);
      UART.println(msg);
      cloudCommandPending = true;
      cloudCommandTime = millis();
    }

  }

  // Reconnect if idle for too long (15s) even if ws->connected() flipped already
  if (cloudConnected && (millis() - wsLastRxTime > 15000)) {
    CONSOLE.println("WS: no RX for 15s, reconnecting");
    if (ws) ws->close();
    cloudConnected = false;
    wsNextConnectTime = millis() + 2000;
  }
}

#endif // USE_CLOUD
