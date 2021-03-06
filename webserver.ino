
/*
webserver.ino - webserver for Sonoff-Tasmota

Copyright (C) 2017  Theo Arends

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_WEBSERVER
/*********************************************************************************************\
* Web server and WiFi Manager
*
* Enables configuration and reconfiguration of WiFi credentials using a Captive Portal
* Based on source by AlexT (https://github.com/tzapu)
\*********************************************************************************************/

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

const char HTML_HEAD[] PROGMEM =
"<!DOCTYPE html><html lang=\"en\" class=\"\">"
"<head>"
"<meta charset='utf-8'>"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=no\"/>"
"<title>{v}</title>"

"<script>"
"var cn,x,lt;"
"cn=120;"
"x=null;"                  // Allow for abortion
"function u(){"
"if(cn>=0){"
"document.getElementById('t').innerHTML='Restart in '+cn+' seconds';"
"cn--;"
"setTimeout(u,1000);"
"}"
"}"
"function c(l){"
"document.getElementById('s1').value=l.innerText||l.textContent;"
"document.getElementById('p1').focus();"
"}"
"function la(p){"
"var a='';"
"if(la.arguments.length==1){"
"a=p;"
"clearTimeout(lt);"
"}"
"if(x!=null){x.abort();}"    // Abort if no response within 2 seconds (happens on restart 1)
"x=new XMLHttpRequest();"
"x.onreadystatechange=function(){"
"if(x.readyState==4&&x.status==200){"
"document.getElementById('l1').innerHTML=x.responseText;"
"}"
"};"
"x.open('GET','ay'+a,true);"
"x.send();"
"lt=setTimeout(la,2345);"
"}"
"function lb(p){"
"la('?d='+p);"
"}"
"</script>"

"<style>"
"body{text-align:center;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;color:#fff;text-shadow:0px 0.05rem 0.1rem rgba(0,0,0,0.5);background-color:#333;}"
"h1,h2,h3{margin-bottom: 0.rem;font-weight:500;line-height:1.1;}"
"div,fieldset,input,select{padding:5px;font-size:1em;}"
"input{width:95%;}"
"select{width:100%;}"
"textarea{resize:none;width:98%;height:318px;padding:5px;overflow:auto;}"
"td{padding:0px;}"
"button{padding:.5rem;font-weight:bold;color:#fff;background-color:#868e96;border:.05rem solid #868e96;border-radius:.3rem;line-height:1.5;font-size:1.05rem;width:100%;-webkit-transition-duration:0.4s;transition-duration:0.4s;}"
"button:hover{background-color:#727b84;border-color:#6c757d}"
".q{float:right;width:200px;text-align:right;}"
"</style>"

"</head>"
"<body>"
"<div style='text-align:left;display:inline-block;min-width:320px;'>"
"<div style='text-align:center;'><h3>{ha} Module</h3><h2>{h}</h2></div>";
const char HTML_SCRIPT_MODULE[] PROGMEM =
"var os;"
"function sk(s,g){"
"var o=os.replace(\"value='\"+s+\"'\",\"selected value='\"+s+\"'\");"
"document.getElementById('g'+g).innerHTML=o;"
"}"
"function sl(){"
"var o0=\"";
const char HTML_MSG_RSTRT[] PROGMEM =
"<br/><div style='text-align:center;'>Device will restart in a few seconds</div><br/>";
const char HTML_BTN_MENU1[] PROGMEM =
"<br/><form action='cn' method='get'><button>Configuration</button></form>"
//"<br/><form action='in' method='get'><button>Information</button></form>"
//"<br/><form action='up' method='get'><button>Firmware upgrade</button></form>"
//"<br/><form action='cs' method='get'><button>Console</button></form>"
;
const char HTML_BTN_RSTRT[] PROGMEM =
"<br/><form action='rb' method='get' onsubmit='return confirm(\"Confirm Restart\");'><button>Restart</button></form>";
const char HTML_BTN_MENU2[] PROGMEM =
"<br/><form action='md' method='get'><button>Configure Module</button></form>"
"<br/><form action='w0' method='get'><button>Configure WiFi</button></form>";
const char HTML_BTN_MENU3A[] PROGMEM =
"<br/><form action='mq' method='get'><button>Configure MQTT</button></form>";
const char HTML_BTN_MENU3B[] PROGMEM =
"<br/><form action='bl' method='get'><button>Configure BLYNK</button></form>";
const char HTML_BTN_MENU4[] PROGMEM =
//"<br/><form action='lg' method='get'><button>Configure Logging</button></form>"
//"<br/><form action='co' method='get'><button>Configure Other</button></form>"
"<br/><form action='rt' method='get' onsubmit='return confirm(\"Confirm Reset Configuration\");'><button>Reset Configuration</button></form>"
//"<br/><form action='dl' method='get'><button>Backup Configuration</button></form>"
//"<br/><form action='rs' method='get'><button>Restore Configuration</button></form>"
;
const char HTML_BTN_MAIN[] PROGMEM =
"<br/><br/><form action='.' method='get'><button>Main menu</button></form>";
const char HTML_BTN_CONF[] PROGMEM =
"<br/><br/><form action='cn' method='get'><button>Configuration menu</button></form>";
const char HTML_FORM_MODULE[] PROGMEM =
"<fieldset><legend><b>&nbsp;Module parameters&nbsp;</b></legend><form method='get' action='sv'>"
"<input id='w' name='w' value='6' hidden><input id='r' name='r' value='1' hidden>"
"<br/><b>Module type</b> ({mt})<br/><select id='mt' name='mt'>";
const char HTML_RELAY_HOLDTIME[] PROGMEM =
"<br/><b>Relay hold time (0-3600 seconds)</b><br/><input id='ht' name='ht' length=4 placeholder='0=disable' value='%d'><br/>";
const char HTML_LNK_ITEM[] PROGMEM =
"<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q'>{i} {r}%</span></div>";
const char HTML_LNK_SCAN[] PROGMEM =
"<div><a href='/w1'>Scan for wifi networks</a></div><br/>";
const char HTML_FORM_MQTT[] PROGMEM =
"<fieldset><legend><b>&nbsp;MQTT parameters&nbsp;</b></legend><form method='get' action='sv'>"
"<input id='w' name='w' value='2' hidden><input id='r' name='r' value='1' hidden>"
"<br/><b>Host</b> (" MQTT_HOST ")<br/><input id='mh' name='mh' length=32 placeholder='" MQTT_HOST" ' value='{m1}'><br/>"
"<br/><b>Port</b> (" STR(MQTT_PORT) ")<br/><input id='ml' name='ml' length=5 placeholder='" STR(MQTT_PORT) "' value='{m2}'><br/>"
"<br/><b>Client Id</b> ({m0})<br/><input id='mc' name='mc' length=32 placeholder='" MQTT_CLIENT_ID "' value='{m3}'><br/>"
//"<br/><b>User</b> (" MQTT_USER ")<br/><input id='mu' name='mu' length=32 placeholder='" MQTT_USER "' value='{m4}'><br/>"
//"<br/><b>Password</b><br/><input id='mp' name='mp' length=32 type='password' placeholder='" MQTT_PASS "' value='{m5}'><br/>"
"<br/><b>Topic</b> = %topic% (" MQTT_TOPIC ")<br/><input id='mt' name='mt' length=32 placeholder='" MQTT_TOPIC" ' value='{m6}'><br/>"
//"<br/><b>Full Topic</b> (" MQTT_FULLTOPIC ")<br/><input id='mf' name='mf' length=80 placeholder='" MQTT_FULLTOPIC" ' value='{m7}'><br/>"
;
const char HTML_FORM_BLYNK[] PROGMEM =
"<fieldset><legend><b>&nbsp;BLYNK parameters&nbsp;</b></legend><form method='get' action='sv'>"
"<input id='w' name='w' value='4' hidden><input id='r' name='r' value='1' hidden>"
"<br/><b>Token</b> (" BLYNK_TOKEN ")<br/><input id='mt' name='mt' length=32 placeholder='" BLYNK_TOKEN" ' value='{m1}'><br/>"
"<br/><b>Server</b> (" BLYNK_SERVER ")<br/><input id='mh' name='mh' length=32 placeholder='" BLYNK_SERVER" ' value='{m2}'><br/>"
"<br/><b>Port</b> (" STR(BLYNK_PORT) ")<br/><input id='ml' name='ml' length=5 placeholder='" STR(BLYNK_PORT) "' value='{m3}'><br/>";
const char HTML_FORM_END[] PROGMEM =
"<br/><button type='submit'>Save</button></form></fieldset>";
const char HTML_FORM_RST[] PROGMEM =
"<div id='f1' name='f1' style='display:block;'>"
"<fieldset><legend><b>&nbsp;Restore configuration&nbsp;</b></legend>";
const char HTML_FORM_CMND[] PROGMEM =
"<br/><textarea readonly id='t1' name='t1' cols='" STR(MESSZ) "' wrap='off'></textarea><br/><br/>"
"<form method='get' onsubmit='return l(1);'>"
"<input style='width:98%' id='c1' name='c1' length='99' placeholder='Enter command' autofocus><br/>"
//  "<br/><button type='submit'>Send command</button>"
"</form>";
const char HTML_TABLE100[] PROGMEM =
"<table style='width:100%'>";
const char HTML_COUNTER[] PROGMEM =
"<br/><div id='t' name='t' style='text-align:center;'></div>";
const char HTML_SNS_TEMP[] PROGMEM =
"<tr><th>%s Temperature</th><td>%s&deg;%c</td></tr>";
const char HTML_SNS_HUM[] PROGMEM =
"<tr><th>%s Humidity</th><td>%s%</td></tr>";
const char HTML_SNS_PRESSURE[] PROGMEM =
"<tr><th>%s Pressure</th><td>%s hPa</td></tr>";
const char HTML_SNS_LIGHT[] PROGMEM =
"<tr><th>%s Light</th><td>%d%</td></tr>";
const char HTML_SNS_NOISE[] PROGMEM =
"<tr><th>%s Noise</th><td>%d%</td></tr>";
const char HTML_SNS_DUST[] PROGMEM =
"<tr><th>%s Air quality</th><td>%d%</td></tr>";
const char HTML_END[] PROGMEM =
"</div>"
"</body>"
"</html>";

const char HDR_CTYPE_PLAIN[] PROGMEM = "text/plain";
const char HDR_CTYPE_HTML[] PROGMEM = "text/html";
const char HDR_CTYPE_XML[] PROGMEM = "text/xml";
const char HDR_CTYPE_JSON[] PROGMEM = "application/json";
const char HDR_CTYPE_STREAM[] PROGMEM = "application/octet-stream";

#define DNS_PORT 53
enum http_t { HTTP_OFF, HTTP_USER, HTTP_ADMIN, HTTP_MANAGER };

DNSServer *dnsServer;
ESP8266WebServer *webServer;

boolean _removeDuplicateAPs = true;
int _minimumQuality = -1;
uint8_t _httpflag = HTTP_OFF;
uint8_t _uploaderror = 0;
uint8_t _uploadfiletype;
uint8_t _colcount;

void startWebserver(int type)
{
	if (!_httpflag) {
		if (!webServer) {
			webServer = new ESP8266WebServer((HTTP_MANAGER == type) ? 80 : WEB_PORT);
			webServer->on("/", handleRoot);
			webServer->on("/cn", handleConfig);
			webServer->on("/md", handleModule);
			//webServer->on("/w1", handleWifi1);
			//webServer->on("/w0", handleWifi0);
#ifdef USE_MQTT
			webServer->on("/mq", handleMqtt);
#endif // USE_MQTT
#ifdef USE_BLYNK
			webServer->on("/bl", handleBlynk);
#endif // USE_BLYNK
			//webServer->on("/lg", handleLog);
			//webServer->on("/co", handleOther);
			//webServer->on("/dl", handleDownload);
			webServer->on("/sv", handleSave);
			//webServer->on("/rs", handleRestore);
			webServer->on("/rt", handleReset);
			//webServer->on("/up", handleUpgrade);
			//webServer->on("/u1", handleUpgradeStart);  // OTA
			//webServer->on("/u2", HTTP_POST, handleUploadDone, handleUploadLoop);
			//webServer->on("/cm", handleCmnd);
			//webServer->on("/cs", handleConsole);
			//webServer->on("/ax", handleAjax);
			webServer->on("/ay", handleAjax2);
			//webServer->on("/in", handleInfo);
			webServer->on("/rb", handleRestart);
			webServer->on("/fwlink", handleRoot);  // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
			webServer->onNotFound(handleNotFound);
		}
		webServer->begin(); // Web server start
	}
	if (type) _httpflag = type;
}

void stopWebserver()
{
	if (_httpflag) {
		webServer->close();
		_httpflag = HTTP_OFF;
	}
}

void beginWifiManager()
{
	// setup AP
	if ((WL_CONNECTED == WiFi.status()) && (static_cast<uint32_t>(WiFi.localIP()) != 0)) {
		WiFi.mode(WIFI_AP_STA);
	}
	else {
		WiFi.mode(WIFI_AP);
	}

	stopWebserver();

	dnsServer = new DNSServer();
	WiFi.softAP(Hostname);
	delay(500); // Without delay I've seen the IP address blank
				/* Setup the DNS server redirecting all the domains to the apIP */
	dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

	startWebserver(HTTP_MANAGER);
}

void pollDnsWeb()
{
	if (dnsServer) {
		dnsServer->processNextRequest();
	}
	if (webServer) {
		webServer->handleClient();
	}
}

void setHeader()
{
	webServer->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	webServer->sendHeader(F("Pragma"), F("no-cache"));
	webServer->sendHeader(F("Expires"), F("-1"));
}

void showPage(String &page)
{
	// TODO: authentication
	//if ((HTTP_ADMIN == _httpflag) && (sysCfg.web_password[0] != 0) && !webServer->authenticate(WEB_USERNAME, sysCfg.web_password)) {
	//	return webServer->requestAuthentication();
	//}
	page.replace(F("{ha}"), modules[settings.module]);
	//page.replace(F("{h}"), sysCfg.friendlyname[0]);
	page.replace(F("{h}"), Hostname);
	//if (HTTP_MANAGER == _httpflag) {
	//	if (WIFI_configCounter()) {
	//		page.replace(F("<body>"), F("<body onload='u()'>"));
	//		page += FPSTR(HTTP_COUNTER);
	//	}
	//}
	page += FPSTR(HTML_END);
	setHeader();
	webServer->send(200, FPSTR(HDR_CTYPE_HTML), page);
}

void handleRoot()
{
	if (captivePortal()) { // If captive portal redirect instead of displaying the page.
		return;
	}

	//if (HTTP_MANAGER == _httpflag) {
	//	handleWifi0();
	//}
	//else 
	{
		char stemp[10], line[100];
		String page = FPSTR(HTML_HEAD);
		page.replace(F("{v}"), F("Main menu"));
		page.replace(F("<body>"), F("<body onload='la()'>"));

		page += F("<div id='l1' name='l1'></div>");
		if (MaxRelay) {
			//if (sfl_flg) {
			//	snprintf_P(line, sizeof(line), PSTR("<input type='range' min='1' max='100' value='%d' onchange='lb(value)'>"),
			//		sysCfg.led_dimmer[0]);
			//	page += line;
			//}
			page += FPSTR(HTML_TABLE100);
			page += F("<tr>");
			for (byte idx = 1; idx <= MaxRelay; idx++) {
				snprintf_P(stemp, sizeof(stemp), PSTR(" %d"), idx);
				snprintf_P(line, sizeof(line), PSTR("<td style='width:%d%'><button onclick='la(\"?o=%d\");'>Toggle%s</button></td>"),
					100 / MaxRelay, idx, (MaxRelay > 1) ? stemp : "");
				page += line;
			}
			page += F("</tr></table>");
		}
		//if (SONOFF_BRIDGE == sysCfg.module) {
		//	page += FPSTR(HTTP_TABLE100);
		//	page += F("<tr>");
		//	byte idx = 0;
		//	for (byte i = 0; i < 4; i++) {
		//		if (idx > 0) {
		//			page += F("</tr><tr>");
		//		}
		//		for (byte j = 0; j < 4; j++) {
		//			idx++;
		//			snprintf_P(line, sizeof(line), PSTR("<td style='width:25%'><button onclick='la(\"?k=%d\");'>%d</button></td>"),
		//				idx, idx);
		//			page += line;
		//		}
		//	}
		//	page += F("</tr></table>");
		//}

		if (HTTP_ADMIN == _httpflag) {
			page += FPSTR(HTML_BTN_MENU1);
			page += FPSTR(HTML_BTN_RSTRT);
		}
		showPage(page);
	}
}

void handleAjax2()
{
	char svalue[50];

	if (strlen(webServer->arg("o").c_str())) {
		do_cmnd_power(atoi(webServer->arg("o").c_str()), 2);
	}
	if (strlen(webServer->arg("d").c_str())) {
		snprintf_P(svalue, sizeof(svalue), PSTR("dimmer %s"), webServer->arg("d").c_str());
		do_cmnd(svalue);
	}
	if (strlen(webServer->arg("k").c_str())) {
		snprintf_P(svalue, sizeof(svalue), PSTR("rfkey%s"), webServer->arg("k").c_str());
		do_cmnd(svalue);
	}

	String tpage = "";
	tpage += sensor_webPresent();
	//tpage += counter_webPresent();
	if (analog_flag) {
		snprintf_P(svalue, sizeof(svalue), PSTR("<tr><th>Analog (A0)</th><td>%d</td></tr>"), getAdc0());
		tpage += svalue;
	}
	//if (hlw_flg) {
	//	tpage += hlw_webPresent();
	//}
	//if (SONOFF_SC == sysCfg.module) {
	//	tpage += sc_webPresent();
	//}
	String page = "";
	if (tpage.length() > 0) {
		page += FPSTR(HTML_TABLE100);
		page += tpage;
		page += F("</table>");
	}
	char line[120];
	if (MaxRelay) {
		page += FPSTR(HTML_TABLE100);
		page += F("<tr>");
		for (byte idx = 1; idx <= MaxRelay; idx++) {
			snprintf_P(line, sizeof(line), PSTR("<td style='width:%d%'><div style='text-align:center;font-weight:bold;font-size:%dpx'>%s</div></td>"),
				100 / MaxRelay, 70 - (MaxRelay * 8), getStateText(digitalRead(settings.relayPins[idx - 1])));
			page += line;
		}
		page += F("</tr></table>");
	}
	webServer->send(200, FPSTR(HDR_CTYPE_HTML), page);
}

boolean httpUser()
{
	boolean status = (HTTP_USER == _httpflag);
	if (status) {
		handleRoot();
	}
	return status;
}

void handleConfig()
{
	if (httpUser()) {
		return;
	}
	String page = FPSTR(HTML_HEAD);
	page.replace(F("{v}"), F("Configuration"));
	page += FPSTR(HTML_BTN_MENU2);
#ifdef USE_MQTT
	page += FPSTR(HTML_BTN_MENU3A);
#endif // USE_MQTT
#ifdef USE_BLYNK
	page += FPSTR(HTML_BTN_MENU3B);
#endif // USE_BLYNK
	page += FPSTR(HTML_BTN_MENU4);
	page += FPSTR(HTML_BTN_MAIN);
	showPage(page);
}

void handleModule()
{
	if (httpUser()) {
		return;
	}
	char stemp[20], line[128];

	String page = FPSTR(HTML_HEAD);
	page.replace(F("{v}"), F("Config module"));
	page += FPSTR(HTML_FORM_MODULE);

	snprintf_P(stemp, sizeof(stemp), modules[0]);
	page.replace(F("{mt}"), stemp);

	for (byte idx = 0; idx < MAX_MODULE; idx++) {
		snprintf_P(stemp, sizeof(stemp), modules[idx]);
		snprintf_P(line, sizeof(line), PSTR("<option%s value='%d'>%02d %s</option>"), (idx == settings.module) ? " selected" : "", idx, idx + 1, stemp);
		page += line;
	}
	page += F("</select></br>");

	for (byte i = 0; i < MAX_SENSOR; i++) {
		if (settings.sensorPins[i] != NOT_A_PIN) {
			snprintf_P(line, sizeof(line), PSTR("<br/><b>GPIO%d</b><select id='g%d' name='g%d'>"), settings.sensorPins[i], i, i);
			page += line;
			for (byte v = 0; v < SENSOR_END; v++) {
				snprintf_P(stemp, sizeof(stemp), sensors[v]);
				snprintf_P(line, sizeof(line), PSTR("<option%s value='%d'>%02d %s</option>"), (v == settings.sensorModes[i]) ? " selected" : "", v, v, stemp);
				page += line;
			}
			page += F("</select></br>");
		}
	}
	snprintf_P(line, sizeof(line), HTML_RELAY_HOLDTIME, settings.relayHoldTime);
	page += line;

	page += FPSTR(HTML_FORM_END);
	page += FPSTR(HTML_BTN_CONF);
	showPage(page);
}

#ifdef USE_MQTT
void handleMqtt()
{
	if (httpUser()) {
		return;
	}

	String page = FPSTR(HTML_HEAD);
	page.replace(F("{v}"), F("Configure MQTT"));
	page += FPSTR(HTML_FORM_MQTT);
	char str[sizeof(settings.mqttClientID)];
	getClient(str, MQTT_CLIENT_ID, sizeof(settings.mqttClientID));
	page.replace(F("{m0}"), str);
	page.replace(F("{m1}"), settings.mqttHost);
	page.replace(F("{m2}"), String(settings.mqttPort));
	page.replace(F("{m3}"), settings.mqttClientID);
	//page.replace(F("{m4}"), (sysCfg.mqtt_user[0] == '\0') ? "0" : sysCfg.mqtt_user);
	//page.replace(F("{m5}"), (sysCfg.mqtt_pwd[0] == '\0') ? "0" : sysCfg.mqtt_pwd);
	page.replace(F("{m6}"), settings.mqttTopic);
	//page.replace(F("{m7}"), sysCfg.mqtt_fulltopic);
	page += FPSTR(HTML_FORM_END);
	page += FPSTR(HTML_BTN_CONF);
	showPage(page);
}
#endif // USE_MQTT

#ifdef USE_BLYNK
void handleBlynk()
{
	if (httpUser()) {
		return;
	}

	String page = FPSTR(HTML_HEAD);
	page.replace(F("{v}"), F("Configure BLYNK"));
	page += FPSTR(HTML_FORM_BLYNK);
	page.replace(F("{m1}"), settings.blynkToken);
	page.replace(F("{m2}"), settings.blynkServer);
	page.replace(F("{m3}"), String(settings.blynkPort));
	page += FPSTR(HTML_FORM_END);
	page += FPSTR(HTML_BTN_CONF);
	showPage(page);
}
#endif // USE_BLYNK

void handleSave()
{
	if (httpUser()) {
		return;
	}

	char stemp[TOPSZ];
	char stemp2[TOPSZ];
	byte what = 0;
	byte restart;
	String result = "";

	if (strlen(webServer->arg("w").c_str())) {
		what = atoi(webServer->arg("w").c_str());
	}
	switch (what) {
	case 1:
		//strlcpy(sysCfg.hostname, (!strlen(webServer->arg("h").c_str())) ? WIFI_HOSTNAME : webServer->arg("h").c_str(), sizeof(sysCfg.hostname));
		//if (strstr(sysCfg.hostname, "%")) {
		//	strlcpy(sysCfg.hostname, WIFI_HOSTNAME, sizeof(sysCfg.hostname));
		//}
		//strlcpy(sysCfg.sta_ssid[0], (!strlen(webServer->arg("s1").c_str())) ? STA_SSID1 : webServer->arg("s1").c_str(), sizeof(sysCfg.sta_ssid[0]));
		//strlcpy(sysCfg.sta_pwd[0], (!strlen(webServer->arg("p1").c_str())) ? STA_PASS1 : webServer->arg("p1").c_str(), sizeof(sysCfg.sta_pwd[0]));
		//strlcpy(sysCfg.sta_ssid[1], (!strlen(webServer->arg("s2").c_str())) ? STA_SSID2 : webServer->arg("s2").c_str(), sizeof(sysCfg.sta_ssid[1]));
		//strlcpy(sysCfg.sta_pwd[1], (!strlen(webServer->arg("p2").c_str())) ? STA_PASS2 : webServer->arg("p2").c_str(), sizeof(sysCfg.sta_pwd[1]));
		//snprintf_P(log, sizeof(log), PSTR("HTTP: Wifi Hostname %s, SSID1 %s, Password1 %s, SSID2 %s, Password2 %s"),
		//	sysCfg.hostname, sysCfg.sta_ssid[0], sysCfg.sta_pwd[0], sysCfg.sta_ssid[1], sysCfg.sta_pwd[1]);
		//addLog(LOG_LEVEL_INFO, log);
		result += F("<br/>Trying to connect device to network<br/>If it fails reconnect to try again");
		break;
#ifdef USE_MQTT
	case 2: // MQTT
		strlcpy(stemp, (!strlen(webServer->arg("mt").c_str())) ? MQTT_TOPIC : webServer->arg("mt").c_str(), sizeof(stemp));
		mqttfy(0, stemp);
		//strlcpy(stemp2, (!strlen(webServer->arg("mf").c_str())) ? MQTT_FULLTOPIC : webServer->arg("mf").c_str(), sizeof(stemp2));
		//mqttfy(1, stemp2);
		//if ((strcmp(stemp, sysCfg.mqtt_topic)) || (strcmp(stemp2, sysCfg.mqtt_fulltopic))) {
		//	mqtt_publish_topic_P(2, PSTR("LWT"), (sysCfg.flag.mqtt_offline) ? "Offline" : "", true);  // Offline or remove previous retained topic
		//}
		strlcpy(settings.mqttTopic, stemp, sizeof(settings.mqttTopic));
		//strlcpy(sysCfg.mqtt_fulltopic, stemp2, sizeof(sysCfg.mqtt_fulltopic));
		strlcpy(settings.mqttHost, (!strlen(webServer->arg("mh").c_str())) ? MQTT_HOST : webServer->arg("mh").c_str(), sizeof(settings.mqttHost));
		settings.mqttPort = (!strlen(webServer->arg("ml").c_str())) ? MQTT_PORT : atoi(webServer->arg("ml").c_str());
		strlcpy(settings.mqttClientID, (!strlen(webServer->arg("mc").c_str())) ? MQTT_CLIENT_ID : webServer->arg("mc").c_str(), sizeof(settings.mqttClientID));
		//strlcpy(sysCfg.mqtt_user, (!strlen(webServer->arg("mu").c_str())) ? MQTT_USER : (!strcmp(webServer->arg("mu").c_str(), "0")) ? "" : webServer->arg("mu").c_str(), sizeof(sysCfg.mqtt_user));
		//strlcpy(sysCfg.mqtt_pwd, (!strlen(webServer->arg("mp").c_str())) ? MQTT_PASS : (!strcmp(webServer->arg("mp").c_str(), "0")) ? "" : webServer->arg("mp").c_str(), sizeof(sysCfg.mqtt_pwd));
		Serial.print("HTTP: MQTT Host ");
		Serial.print(settings.mqttHost);
		Serial.print(", Port ");
		Serial.print(settings.mqttPort);
		Serial.print(", Client ");
		Serial.print(settings.mqttClientID);
		Serial.print(", Topic ");
		Serial.println(settings.mqttTopic);
		break;
#endif // USE_MQTT
	case 3:
		//sysCfg.seriallog_level = (!strlen(webServer->arg("ls").c_str())) ? SERIAL_LOG_LEVEL : atoi(webServer->arg("ls").c_str());
		//sysCfg.weblog_level = (!strlen(webServer->arg("lw").c_str())) ? WEB_LOG_LEVEL : atoi(webServer->arg("lw").c_str());
		//sysCfg.syslog_level = (!strlen(webServer->arg("ll").c_str())) ? SYS_LOG_LEVEL : atoi(webServer->arg("ll").c_str());
		//syslog_level = sysCfg.syslog_level;
		//syslog_timer = 0;
		//strlcpy(sysCfg.syslog_host, (!strlen(webServer->arg("lh").c_str())) ? SYS_LOG_HOST : webServer->arg("lh").c_str(), sizeof(sysCfg.syslog_host));
		//sysCfg.syslog_port = (!strlen(webServer->arg("lp").c_str())) ? SYS_LOG_PORT : atoi(webServer->arg("lp").c_str());
		//sysCfg.tele_period = (!strlen(webServer->arg("lt").c_str())) ? TELE_PERIOD : atoi(webServer->arg("lt").c_str());
		//snprintf_P(log, sizeof(log), PSTR("HTTP: Logging Seriallog %d, Weblog %d, Syslog %d, Host %s, Port %d, TelePeriod %d"),
		//	sysCfg.seriallog_level, sysCfg.weblog_level, sysCfg.syslog_level, sysCfg.syslog_host, sysCfg.syslog_port, sysCfg.tele_period);
		//addLog(LOG_LEVEL_INFO, log);
		break;
#ifdef USE_BLYNK
	case 4: // BLYNK
		strlcpy(settings.blynkToken, (!strlen(webServer->arg("mt").c_str())) ? BLYNK_TOKEN : webServer->arg("mt").c_str(), sizeof(settings.blynkToken));
		strlcpy(settings.blynkServer, (!strlen(webServer->arg("mh").c_str())) ? MQTT_HOST : webServer->arg("mh").c_str(), sizeof(settings.blynkServer));
		settings.blynkPort = (!strlen(webServer->arg("ml").c_str())) ? MQTT_PORT : atoi(webServer->arg("ml").c_str());
		Serial.print("HTTP: BLYNK Token ");
		Serial.print(settings.blynkToken);
		Serial.print(", Server ");
		Serial.print(settings.blynkServer);
		Serial.print(", Port ");
		Serial.println(settings.blynkPort);
		break;
#endif // USE_BLYNK
	case 5:
		//		strlcpy(sysCfg.web_password, (!strlen(webServer->arg("p1").c_str())) ? WEB_PASSWORD : (!strcmp(webServer->arg("p1").c_str(), "0")) ? "" : webServer->arg("p1").c_str(), sizeof(sysCfg.web_password));
		//		sysCfg.flag.mqtt_enabled = webServer->hasArg("b1");
		//#ifdef USE_EMULATION
		//		sysCfg.flag.emulation = (!strlen(webServer->arg("b2").c_str())) ? 0 : atoi(webServer->arg("b2").c_str());
		//#endif  // USE_EMULATION
		//		strlcpy(sysCfg.friendlyname[0], (!strlen(webServer->arg("a1").c_str())) ? FRIENDLY_NAME : webServer->arg("a1").c_str(), sizeof(sysCfg.friendlyname[0]));
		//		strlcpy(sysCfg.friendlyname[1], (!strlen(webServer->arg("a2").c_str())) ? FRIENDLY_NAME"2" : webServer->arg("a2").c_str(), sizeof(sysCfg.friendlyname[1]));
		//		strlcpy(sysCfg.friendlyname[2], (!strlen(webServer->arg("a3").c_str())) ? FRIENDLY_NAME"3" : webServer->arg("a3").c_str(), sizeof(sysCfg.friendlyname[2]));
		//		strlcpy(sysCfg.friendlyname[3], (!strlen(webServer->arg("a4").c_str())) ? FRIENDLY_NAME"4" : webServer->arg("a4").c_str(), sizeof(sysCfg.friendlyname[3]));
		//		snprintf_P(log, sizeof(log), PSTR("HTTP: Other MQTT Enable %s, Emulation %d, Friendly Names %s, %s, %s and %s"),
		//			getStateText(sysCfg.flag.mqtt_enabled), sysCfg.flag.emulation, sysCfg.friendlyname[0], sysCfg.friendlyname[1], sysCfg.friendlyname[2], sysCfg.friendlyname[3]);
		//		addLog(LOG_LEVEL_INFO, log);
		break;
	case 6: // Module
		byte new_module = (!strlen(webServer->arg("mt").c_str())) ? 0 : atoi(webServer->arg("mt").c_str());
		byte new_modflg = (settings.module != new_module);
		settings.module = new_module;
		if (new_modflg) {
			module_setup();
		}
		else {
			for (byte i = 0; i < MAX_SENSOR; i++) {
				snprintf_P(stemp, sizeof(stemp), PSTR("g%d"), i);
				settings.sensorModes[i] = (!strlen(webServer->arg(stemp).c_str())) ? 0 : atoi(webServer->arg(stemp).c_str());
			}
			int holdTime = (!strlen(webServer->arg("ht").c_str())) ? 0 : atoi(webServer->arg("ht").c_str());
			if (holdTime < 0) {
				holdTime = 0;
			}
			else if (holdTime > 3600) {
				holdTime = 3600;
			}
			settings.relayHoldTime = holdTime;
			break;
		}
	}

	cfg_save();

	restart = (!strlen(webServer->arg("r").c_str())) ? 1 : atoi(webServer->arg("r").c_str());
	if (restart) {
		String page = FPSTR(HTML_HEAD);
		page.replace(F("{v}"), F("Save parameters"));
		page += F("<div style='text-align:center;'><b>Parameters saved</b><br/>");
		page += result;
		page += F("</div>");
		page += FPSTR(HTML_MSG_RSTRT);
		if (HTTP_MANAGER == _httpflag) {
			_httpflag = HTTP_ADMIN;
		}
		else {
			page += FPSTR(HTML_BTN_MAIN);
		}
		showPage(page);

		restartflag = 2;
	}
	else {
		handleConfig();
	}
}

void handleReset()
{
	if (httpUser()) {
		return;
	}

	char svalue[16];  // was MESSZ

	String page = FPSTR(HTML_HEAD);
	page.replace(F("{v}"), F("Default parameters"));
	page += F("<div style='text-align:center;'>Parameters reset to default</div>");
	page += FPSTR(HTML_MSG_RSTRT);
	page += FPSTR(HTML_BTN_MAIN);
	showPage(page);

	restartflag = 200;
}

void handleCmnd()
{
	if (httpUser()) {
		return;
	}
	char svalue[INPUT_BUFFER_SIZE];  // big to serve Backlog

	uint8_t valid = 1;
	// TODO: Password check
	//if (sysCfg.web_password[0] != 0) {
	//	if (!(!strcmp(webServer->arg("user").c_str(), WEB_USERNAME) && !strcmp(webServer->arg("password").c_str(), sysCfg.web_password))) {
	//		valid = 0;
	//	}
	//}

	String message = "";
	if (valid) {
		//byte curridx = logidx;
		if (strlen(webServer->arg("cmnd").c_str())) {
			//      snprintf_P(svalue, sizeof(svalue), webServer->arg("cmnd").c_str());
			snprintf_P(svalue, sizeof(svalue), PSTR("%s"), webServer->arg("cmnd").c_str());
			//byte syslog_now = syslog_level;
			//syslog_level = 0;  // Disable UDP syslog to not trigger hardware WDT
			do_cmnd(svalue);
			//syslog_level = syslog_now;
		}

		//if (logidx != curridx) {
		//	byte counter = curridx;
		//	do {
		//		if (Log[counter].length()) {
		//			if (message.length()) {
		//				message += F("\n");
		//			}
		//			if (sysCfg.flag.mqtt_enabled) {
		//				// [14:49:36 MQTT: stat/wemos5/RESULT = {"POWER":"OFF"}] > [RESULT = {"POWER":"OFF"}]
		//				//            message += Log[counter].substring(17 + strlen(PUB_PREFIX) + strlen(sysCfg.mqtt_topic));
		//				message += Log[counter].substring(Log[counter].lastIndexOf("/", Log[counter].indexOf("=")) + 1);
		//			}
		//			else {
		//				// [14:49:36 RSLT: RESULT = {"POWER":"OFF"}] > [RESULT = {"POWER":"OFF"}]
		//				message += Log[counter].substring(Log[counter].indexOf(": ") + 2);
		//			}
		//		}
		//		counter++;
		//		if (counter > MAX_LOG_LINES - 1) {
		//			counter = 0;
		//		}
		//	} while (counter != logidx);
		//}
		//else {
		//	message = F("Enable weblog 2 if response expected\n");
		//}
	}
	else {
		message = F("Need user=<username>&password=<password>\n");
	}
	webServer->send(200, FPSTR(HDR_CTYPE_PLAIN), message);
}

void handleRestart()
{
	if (httpUser()) {
		return;
	}
	String page = FPSTR(HTML_HEAD);
	page.replace(F("{v}"), F("Info"));
	page += FPSTR(HTML_MSG_RSTRT);
	if (HTTP_MANAGER == _httpflag) {
		_httpflag = HTTP_ADMIN;
	}
	else {
		page += FPSTR(HTML_BTN_MAIN);
	}
	showPage(page);

	restartflag = 2;
}

/********************************************************************************************/

void handleNotFound()
{
	if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
		return;
	}

	String message = F("File Not Found\n\nURI: ");
	message += webServer->uri();
	message += F("\nMethod: ");
	message += (webServer->method() == HTTP_GET) ? F("GET") : F("POST");
	message += F("\nArguments: ");
	message += webServer->args();
	message += F("\n");
	for (uint8_t i = 0; i < webServer->args(); i++) {
		message += " " + webServer->argName(i) + ": " + webServer->arg(i) + "\n";
	}
	setHeader();
	webServer->send(404, FPSTR(HDR_CTYPE_PLAIN), message);
}

/* Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal()
{
	if ((HTTP_MANAGER == _httpflag) && !isIp(webServer->hostHeader())) {

		webServer->sendHeader(F("Location"), String("http://") + webServer->client().localIP().toString(), true);
		webServer->send(302, FPSTR(HDR_CTYPE_PLAIN), ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
		webServer->client().stop(); // Stop is needed because we sent no content length
		return true;
	}
	return false;
}

/** Is this an IP? */
boolean isIp(String str)
{
	for (uint16_t i = 0; i < str.length(); i++) {
		int c = str.charAt(i);
		if (c != '.' && (c < '0' || c > '9')) {
			return false;
		}
	}
	return true;
}
#endif  // USE_WEBSERVER