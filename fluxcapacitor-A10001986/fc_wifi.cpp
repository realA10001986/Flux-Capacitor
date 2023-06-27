/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor-A10001986
 *
 * WiFi and Config Portal handling
 *
 * -------------------------------------------------------------------
 * License: MIT
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "fc_global.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef FC_MDNS
#include <ESPmDNS.h>
#endif

#include <WiFiManager.h>

#include "fc_audio.h"
#include "fc_settings.h"
#include "fc_wifi.h"
#include "fc_main.h"
#ifdef FC_HAVEMQTT
#include "mqtt.h"
#endif

// If undefined, use the checkbox/dropdown-hacks.
// If defined, go back to standard text boxes
//#define TC_NOCHECKBOXES

// If defined, show the audio file installer field
// Since we invoke the installer now automatically,
// this is no longer needed.
//#define CP_AUDIO_INSTALLER

Settings settings;

IPSettings ipsettings;

WiFiManager wm;

#ifdef FC_HAVEMQTT
WiFiClient mqttWClient;
PubSubClient mqttClient(mqttWClient);
#endif

static const char *aco = "autocomplete='off'";

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_playFLUXSnd("plyFS", "Play continuous flux sound (0=no, 1=yes)", settings.playFLUXsnd, 1, "autocomplete='off' title='Enable to have the device play the flux sound after power-up'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_playFLUXSnd("plyFS", "Play continuous flux sound", settings.playFLUXsnd, 1, "autocomplete='off' title='Check to have the device play the flux sound after power-up' type='checkbox' style='margin-top:3px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_playTTSnd("plyTTS", "Play time travel sounds (0=no, 1=yes)", settings.playTTsnds, 1, "autocomplete='off' title='Enable to have the device play time travel sounds. Disable if other props provide time travel sound.'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_playTTSnd("plyTTS", "Play time travel sounds", settings.playTTsnds, 1, "autocomplete='off' title='Check to have the device play time travel sounds. Uncheck if other props provide time travel sound.' type='checkbox'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useVknob("vKnob", "Use volume knob by default (0=off, 1=on)", settings.useVknob, 1, "autocomplete='off' title='Enable to use volume knob by default'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useVknob("vKnob", "Use volume knob by default", settings.useVknob, 1, "title='Check to use volume knob by default' type='checkbox' style='margin-top:3px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useSknob("sKnob", "Use speed knob by default (0=off, 1=on)", settings.useSknob, 1, "autocomplete='off' title='Enable to use speed knob by default'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useSknob("sKnob", "Use speed knob by default", settings.useSknob, 1, "title='Check to use speed knob by default' type='checkbox'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_TCDpresent("TCDpres", "TCD connected by wire (0=no, 1=yes)", settings.TCDpresent, 1, "autocomplete='off' title='Enable if you have a Time Circuits Display connected via wire'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_TCDpresent("TCDpres", "TCD connected by wire", settings.TCDpresent, 1, "autocomplete='off' title='Check if you have a Time Circuits Display connected via wire' type='checkbox'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_swapBL("swapBL", "Use 'panel light' for box lights (0=no, 1=yes)", settings.usePLforBL, 1, "autocomplete='off' title='Enable if you want to connect your box lights to the Panel Light connector'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_swapBL("swapBL", "Use 'panel light' for box lights", settings.usePLforBL, 1, "autocomplete='off' title='Check if you want to connect your box lights to the Panel Light connector' type='checkbox'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------


#if defined(FC_MDNS) || defined(FC_WM_HAS_MDNS)
#define HNTEXT "Hostname<br><span style='font-size:80%'>The Config Portal is accessible at http://<i>hostname</i>.local<br>(Valid characters: a-z/0-9/-)</span>"
#else
#define HNTEXT "Hostname<br><span style='font-size:80%'>(Valid characters: a-z/0-9/-)</span>"
#endif
WiFiManagerParameter custom_hostName("hostname", HNTEXT, settings.hostName, 31, "pattern='[A-Za-z0-9-]+' placeholder='Example: fluxcapacitor'");
WiFiManagerParameter custom_wifiConRetries("wifiret", "WiFi connection attempts (1-15)", settings.wifiConRetries, 2, "type='number' min='1' max='15' autocomplete='off'", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_wifiConTimeout("wificon", "WiFi connection timeout (7-25[seconds])", settings.wifiConTimeout, 2, "type='number' min='7' max='25'");

#ifdef FC_HAVEMQTT
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useMQTT("uMQTT", "Use Home Assistant (0=no, 1=yes)", settings.useMQTT, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useMQTT("uMQTT", "Use Home Assistant (MQTT 3.1.1)", settings.useMQTT, 1, "type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_mqttServer("ha_server", "<br>Broker IP[:port] or domain[:port]", settings.mqttServer, 79, "pattern='[a-zA-Z0-9.-:]+' placeholder='Example: 192.168.1.5'");
WiFiManagerParameter custom_mqttUser("ha_usr", "User[:Password]", settings.mqttUser, 63, "placeholder='Example: ronald:mySecret'");
#endif // HAVEMQTT

WiFiManagerParameter custom_musHint("<div style='margin:0px;padding:0px'>MusicPlayer</div>");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_shuffle("musShu", "Shuffle at startup (0=no, 1=yes)", settings.shuffle, 1, "autocomplete='off' title='Enable to shuffle playlist at startup'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_shuffle("musShu", "Shuffle at startup", settings.shuffle, 1, "title='Check to shuffle playlist at startup' type='checkbox' style='margin-top:8px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_CfgOnSD("CfgOnSD", "Save volume/speed/IR settings on SD (0=no, 1=yes)<br><span style='font-size:80%'>Enable this to avoid flash wear</span>", settings.CfgOnSD, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_CfgOnSD("CfgOnSD", "Save volume/speed/IR settings on SD<br><span style='font-size:80%'>Check this to avoid flash wear</span>", settings.CfgOnSD, 1, "autocomplete='off' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
//#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
//WiFiManagerParameter custom_sdFrq("sdFrq", "SD clock speed (0=16Mhz, 1=4Mhz)<br><span style='font-size:80%'>Slower access might help in case of problems with SD cards</span>", settings.sdFreq, 1, "autocomplete='off'");
//#else // -------------------- Checkbox hack: --------------
//WiFiManagerParameter custom_sdFrq("sdFrq", "4MHz SD clock speed<br><span style='font-size:80%'>Checking this might help in case of SD card problems</span>", settings.sdFreq, 1, "autocomplete='off' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
//#endif // -------------------------------------------------

#ifdef CP_AUDIO_INSTALLER
WiFiManagerParameter custom_copyAudio("cpAu", "Audio file installation: Write COPY here to copy the original audio files from the SD card to the internal storage.", settings.copyAudio, 6, "autocomplete='off'");
WiFiManagerParameter custom_copyHint("<div style='margin:0px;padding:0px;font-size:80%'>If display shows 'ERROR' when finished, write FORMAT instead of COPY on second attempt.</div>");
#endif

WiFiManagerParameter custom_sectstart_head("<div class='sects'>");
WiFiManagerParameter custom_sectstart("</div><div class='sects'>");
WiFiManagerParameter custom_sectend("</div>");

WiFiManagerParameter custom_sectend_foot("</div><p></p>");

#define TC_MENUSIZE 7
static const char* wifiMenu[TC_MENUSIZE] = {"wifi", "param", "sep", "restart", "update", "sep", "custom" };

//static const char* myHead = "<link rel='shortcut icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAxQTFRFSUpKk491zszD/PGzYuH5fAAAAD5JREFUeNpiYEIDDEwMKACHACOEwwgTYGQGK2NiZoSpYAKJgAmYGUBJmDKooYwwg3Bby8xMuQA+v6ABgAADAGYRALv2zDkbAAAAAElFTkSuQmCC'><script>function getn(x){return document.getElementsByTagName(x)}function ge(x){return document.getElementById(x)}function c(l){ge('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;p=l.nextElementSibling.classList.contains('l');ge('p').disabled=!p;if(p){ge('p').placeholder='';ge('p').focus();}}window.onload=function(){document.title='Flux Capacitor';if(ge('s')&&ge('dns')){aa=document.getElementsByClassName('wrap');if(aa.length>0){aa[0].innerHTML='<img id=\"tcgfx\" class=\"tcgfx\" src=\"\">' + aa[0].innerHTML;}aa=ge('s').parentElement;bb=aa.innerHTML;dd=bb.search('<hr>');ee=bb.search('<button');cc='<div class=\"sects\">'+bb.substring(0,dd)+'</div><div class=\"sects\">'+bb.substring(dd+4,ee)+'</div>'+bb.substring(ee);aa.innerHTML=cc;document.querySelectorAll('a[href=\"#p\"]').forEach((userItem)=>{userItem.onclick=function(){c(this);return false;}});if(aa=ge('s')){aa.oninput=function(){if(this.placeholder.length>0&&this.value.length==0){ge('p').placeholder='********';}}}} if(ge('uploadbin')||window.location.pathname=='/u'||window.location.pathname=='/wifisave'){aa=document.getElementsByClassName('wrap');if(aa.length>0){aa[0].innerHTML='<img id=\"tcgfx\" class=\"tcgfx\" src=\"\">'+aa[0].innerHTML;if((bb=ge('uploadbin'))){aa[0].style.textAlign='center';bb.parentElement.onsubmit=function(){aa=document.getElementById('uploadbin');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}}}aa=getn('H3');if(aa.length>0){aa[0].remove()}aa=getn('H1');if(aa.length>0){aa[0].remove()}}} if(ge('ebnew')){zz=(Math.random()>0.8);dd=document.createElement('div');dd.classList.add('tpm');bb=getn('H3');aa=getn('H1');ff=aa[0].parentNode;ff.style.position='relative';dd.innerHTML='<div class=\"tpm2\"><img src=\"data:image/png;base64,'+(zz?'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAZQTFRFSp1tAAAA635cugAAAAJ0Uk5T/wDltzBKAAAAbUlEQVR42tzXwRGAQAwDMdF/09QQQ24MLkDj77oeTiPA1wFGQiHATOgDGAp1AFOhDWAslAHMhS6AQKgCSIQmgEgoAsiEHoBQqAFIhRaAWCgByIVXAMuAdcA6YBlwALAKePzgd71QAByP71uAAQC+xwvdcFg7UwAAAABJRU5ErkJggg==':'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAZQTFRFSp1tAAAA635cugAAAAJ0Uk5T/wDltzBKAAAAgElEQVR42tzXQQqDABAEwcr/P50P2BBUdMhee6j7+lw8i4BCD8MiQAjHYRAghAh7ADWMMAcQww5jADHMsAYQwwxrADHMsAYQwwxrADHMsAYQwwxrgLgOPwKeAjgrrACcFkYAzgu3AN4C3AV4D3AP4E3AHcDF+8d/YQB4/Pn+CjAAMaIIJuYVQ04AAAAASUVORK5CYII=')+'\" class=\"tpm3\"></div><H1 class=\"tpmh1\"'+(zz?' style=\"margin-left:1.2em\"':'')+'>'+aa[0].innerHTML+'</H1>'+'<H3 class=\"tpmh3\"'+(zz?' style=\"padding-left:4.5em\"':'')+'>'+bb[0].innerHTML+'</div>';bb[0].remove();aa[0].replaceWith(dd);} if((aa=ge('tcgfx'))){aa.src=' data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAMgAAABXCAMAAAB2tvo6AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAYBQTFRF/KQQMJjNept2a7vlU7Dh+WsejHNu/asPq69MW5CTzq4xsXFSyOb1/rsMbI2D0ur3/sIK/bQNhotu91EkZXyOh8jqNY+50os1sJJN+4kX+nsaOKHX9fr9+XIcFZPVtN3y0nM5RKjd+oIZ/J0Sw3NE+5QV+GMgY6/Td8Hn+F0hhaNrRpWp6vX74oYp4/L6/5YNq9jwQJm4+v3+S5+p6Lgckc3sR5fE9LQUSYeq/8kJl6tbmZZdUavWa6GCmtDtRY20Zoun/4QRotTv3ago52cr0148/7kE43grWp6VveHz7K0bQJ/HVIGeUaTL/7IG51cvu7A/enh8/3wTwpJA91cj8cAU/3UV/40P5cUd25srX7XjV5m/T5We7nEk9JQaoWlg/78D3lg1iMHZa5+75PHy9YsavGJN2+/57YQi7p0d+Xcb6fT2838en35e9b0R8lMn98oP7/j8714ozeTk2+zs3b8k7L0Yfa7Oncnjt9vv7I4h3bUk/JgU+48W9sYQ////YdWyCAAAAIB0Uk5T/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////wA4BUtnAAAUcUlEQVR42tRcj3/S1tonJGIpkWglaV1iQibaXEpjSE1FWqjGYkmhE1Y314iyXt81uomOvu/t7qu7l3/9PT9DIHDX917d5vPZxjk5NHy/z3nOr+d5zhIlTTQciw+G9uizlgT/spvKtXc2t31DdD3BVD5fIv2rVx83TvqpLuAjbeo+CxhpfPCZMUoMLaZxNRTKqAK7iDUcrzRSBE8YfgZERrbYvhqTx8eYUYU1g1/uSzLoJ8f1/sy9lBiNTL1/da7kROHZ8TvICnTTpmj+mYmMrMq7uUSOGc3Ihf3UFu0/M5Eh253fJX2OH/fYMePFXzHkwdRQUuw/nshIiI73aWmLmvR4TCtuXPzLtsS0dI4VrdIfTGTmeKfSYLyxcV1tOzHNe/ev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'}}</script><style type='text/css'>body{font-family:-apple-system,BlinkMacSystemFont,system-ui,'Segoe UI',Roboto,'Helvetica Neue',Verdana,Helvetica}H1,H2{margin-top:0px;margin-bottom:0px;text-align:center;}H3{margin-top:0px;margin-bottom:5px;text-align:center;}div.msg{border:1px solid #ccc;border-left-width:15px;border-radius:20px;background:linear-gradient(320deg,rgb(255,255,255) 0%,rgb(235,234,233) 100%);}button{transition-delay:250ms;margin-top:10px;margin-bottom:10px;color:#fff;background-color:#225a98;font-variant-caps:all-small-caps;}button.DD{color:#000;border:4px ridge #999;border-radius:2px;background:#e0c942;background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAADBQTFRF////AAAAMyks8+AAuJYi3NHJo5aQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAbP19EwAAAAh0Uk5T/////////wDeg71ZAAAA4ElEQVR42qSTyxLDIAhF7yChS/7/bwtoFLRNF2UmRr0H8IF4/TBsY6JnQFvTJ8D0ncChb0QGlDvA+hkw/yC4xED2Z2L35xwDRSdqLZpFIOU3gM2ox6mA3tnDPa8UZf02v3q6gKRH/Eyg6JZBqRUCRW++yFYIvCjNFIt9OSC4hol/ItH1FkKRQgAbi0ty9f/F7LM6FimQacPbAdG5zZVlWdfvg+oEpl0Y+jzqIJZ++6fLqlmmnq7biZ4o67lgjBhA0kvJyTww/VK0hJr/LHvBru8PR7Dpx9MT0f8e72lvAQYALlAX+Kfw0REAAAAASUVORK5CYII=');background-repeat:no-repeat;background-origin:content-box;background-size:contain;}br{display:block;font-size:1px;content:''}input[type='checkbox']{display:inline-block;margin-top:10px}input{border:thin inset}small{display:none}em > small{display:inline}form{margin-block-end:0;}.tpm{border:1px solid black;border-radius:5px;padding:0 0 0 0px;min-width:18em;}.tpm2{position:absolute;top:-0.7em;z-index:130;left:0.7em;}.tpm3{width:4em;height:4em;}.tpmh1{font-variant-caps:all-small-caps;margin-left:2em}.tpmh3{background:#000;font-size:0.6em;color:#ffa;padding-left:7em;margin-left:0.5em;margin-right:0.5em;border-radius:5px}.sects{background-color:#eee;border-radius:7px;margin-bottom:20px;padding-bottom:7px;padding-top:7px}.tcgfx{display:block;margin:0px auto 10px auto;}</style>";
static const char* myHead = "<link rel='shortcut icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAxQTFRFSUpKk491zszD/PGzYuH5fAAAAD5JREFUeNpiYEIDDEwMKACHACOEwwgTYGQGK2NiZoSpYAKJgAmYGUBJmDKooYwwg3Bby8xMuQA+v6ABgAADAGYRALv2zDkbAAAAAElFTkSuQmCC'><script>function getn(x){return document.getElementsByTagName(x)}function ge(x){return document.getElementById(x)}function c(l){ge('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;p=l.nextElementSibling.classList.contains('l');ge('p').disabled=!p;if(p){ge('p').placeholder='';ge('p').focus();}}window.onload=function(){xx=false;document.title='Flux Capacitor';if(ge('s')&&ge('dns')){xx=true;xxx=document.title;yyy='Configure WiFi';aa=ge('s').parentElement;bb=aa.innerHTML;dd=bb.search('<hr>');ee=bb.search('<button');cc='<div class=\"sects\">'+bb.substring(0,dd)+'</div><div class=\"sects\">'+bb.substring(dd+4,ee)+'</div>'+bb.substring(ee);aa.innerHTML=cc;document.querySelectorAll('a[href=\"#p\"]').forEach((userItem)=>{userItem.onclick=function(){c(this);return false;}});if(aa=ge('s')){aa.oninput=function(){if(this.placeholder.length>0&&this.value.length==0){ge('p').placeholder='********';}}}}if(ge('uploadbin')||window.location.pathname=='/u'||window.location.pathname=='/wifisave'){xx=true;xxx=document.title;yyy=(window.location.pathname=='/wifisave')?'Configure WiFi':'Firmware update';aa=document.getElementsByClassName('wrap');if(aa.length>0){if((bb=ge('uploadbin'))){aa[0].style.textAlign='center';bb.parentElement.onsubmit=function(){aa=document.getElementById('uploadbin');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}}}aa=getn('H3');if(aa.length>0){aa[0].remove()}aa=getn('H1');if(aa.length>0){aa[0].remove()}}}if(ge('ttrp')||window.location.pathname=='/param'){xx=true;xxx=document.title;yyy='Setup';}if(ge('ebnew')){xx=true;bb=getn('H3');aa=getn('H1');xxx=aa[0].innerHTML;yyy=bb[0].innerHTML;ff=aa[0].parentNode;ff.style.position='relative';}if(xx){zz=(Math.random()>0.8);dd=document.createElement('div');dd.classList.add('tpm0');dd.innerHTML='<div class=\"tpm\"><div class=\"tpm2\"><img src=\"data:image/png;base64,'+(zz?'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAZQTFRFSp1tAAAA635cugAAAAJ0Uk5T/wDltzBKAAAAbUlEQVR42tzXwRGAQAwDMdF/09QQQ24MLkDj77oeTiPA1wFGQiHATOgDGAp1AFOhDWAslAHMhS6AQKgCSIQmgEgoAsiEHoBQqAFIhRaAWCgByIVXAMuAdcA6YBlwALAKePzgd71QAByP71uAAQC+xwvdcFg7UwAAAABJRU5ErkJggg==':'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAZQTFRFSp1tAAAA635cugAAAAJ0Uk5T/wDltzBKAAAAgElEQVR42tzXQQqDABAEwcr/P50P2BBUdMhee6j7+lw8i4BCD8MiQAjHYRAghAh7ADWMMAcQww5jADHMsAYQwwxrADHMsAYQwwxrADHMsAYQwwxrgLgOPwKeAjgrrACcFkYAzgu3AN4C3AV4D3AP4E3AHcDF+8d/YQB4/Pn+CjAAMaIIJuYVQ04AAAAASUVORK5CYII=')+'\" class=\"tpm3\"></div><H1 class=\"tpmh1\"'+(zz?' style=\"margin-left:1.2em\"':'')+'>'+xxx+'</H1>'+'<H3 class=\"tpmh3\"'+(zz?' style=\"padding-left:4.5em\"':'')+'>'+yyy+'</div></div>';}if(ge('ebnew')){bb[0].remove();aa[0].replaceWith(dd);}if((ge('s')&&ge('dns'))||ge('uploadbin')||window.location.pathname=='/u'||window.location.pathname=='/wifisave'||ge('ttrp')||window.location.pathname=='/param'){aa=document.getElementsByClassName('wrap');if(aa.length>0){aa[0].insertBefore(dd,aa[0].firstChild);aa[0].style.position='relative';}}}</script><style type='text/css'>body{font-family:-apple-system,BlinkMacSystemFont,system-ui,'Segoe UI',Roboto,'Helvetica Neue',Verdana,Helvetica}H1,H2{margin-top:0px;margin-bottom:0px;text-align:center;}H3{margin-top:0px;margin-bottom:5px;text-align:center;}div.msg{border:1px solid #ccc;border-left-width:15px;border-radius:20px;background:linear-gradient(320deg,rgb(255,255,255) 0%,rgb(235,234,233) 100%);}button{transition-delay:250ms;margin-top:10px;margin-bottom:10px;color:#fff;background-color:#225a98;font-variant-caps:all-small-caps;}button.DD{color:#000;border:4px ridge #999;border-radius:2px;background:#e0c942;background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAADBQTFRF////AAAAMyks8+AAuJYi3NHJo5aQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAbP19EwAAAAh0Uk5T/////////wDeg71ZAAAA4ElEQVR42qSTyxLDIAhF7yChS/7/bwtoFLRNF2UmRr0H8IF4/TBsY6JnQFvTJ8D0ncChb0QGlDvA+hkw/yC4xED2Z2L35xwDRSdqLZpFIOU3gM2ox6mA3tnDPa8UZf02v3q6gKRH/Eyg6JZBqRUCRW++yFYIvCjNFIt9OSC4hol/ItH1FkKRQgAbi0ty9f/F7LM6FimQacPbAdG5zZVlWdfvg+oEpl0Y+jzqIJZ++6fLqlmmnq7biZ4o67lgjBhA0kvJyTww/VK0hJr/LHvBru8PR7Dpx9MT0f8e72lvAQYALlAX+Kfw0REAAAAASUVORK5CYII=');background-repeat:no-repeat;background-origin:content-box;background-size:contain;}br{display:block;font-size:1px;content:''}input[type='checkbox']{display:inline-block;margin-top:10px}input{border:thin inset}small{display:none}em > small{display:inline}form{margin-block-end:0;}.tpm{border:1px solid black;border-radius:5px;padding:0 0 0 0px;min-width:18em;}.tpm2{position:absolute;top:-0.7em;z-index:130;left:0.7em;}.tpm3{width:4em;height:4em;}.tpmh1{font-variant-caps:all-small-caps;margin-left:2em;}.tpmh3{background:#000;font-size:0.6em;color:#ffa;padding-left:7em;margin-left:0.5em;margin-right:0.5em;border-radius:5px}.sects{background-color:#eee;border-radius:7px;margin-bottom:20px;padding-bottom:7px;padding-top:7px}.tpm0{position:relative;width:350px;margin:0 auto 0 auto;}.headl{margin:0 0 3px 0;padding:0}.cmp0{margin:0;padding:0;}.sel0{font-size:90%;width:auto;margin-left:10px;vertical-align:baseline;}</style>";


static const char* myCustMenu = "<form action='/erase' method='get' onsubmit='return confirm(\"This erases the WiFi config and reboots. The device will restart in access point mode. Are you sure?\");'><button id='ebnew' class='DD'>Erase WiFi Config</button></form><br/><img style='display:block;margin:10px auto 10px auto;' src='data:image/png;base64,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'><div style='font-size:9px;margin-left:auto;margin-right:auto;text-align:center;'>Version " FC_VERSION " (" FC_VERSION_EXTRA ")<br>Powered by A10001986</div>";
// &#x26a0; = warning; &#9762; "radio-active" symbol not rendered properly in many browsers

static int  shouldSaveConfig = 0;
static bool shouldSaveIPConfig = false;
static bool shouldDeleteIPConfig = false;

// WiFi power management in AP mode
bool          wifiInAPMode = false;
bool          wifiAPIsOff = false;
unsigned long wifiAPModeNow;
unsigned long wifiAPOffDelay = 0;     // default: never

// WiFi power management in STA mode
bool          wifiIsOff = false;
unsigned long wifiOnNow = 0;
unsigned long wifiOffDelay     = 0;   // default: never
unsigned long origWiFiOffDelay = 0;

#ifdef FC_HAVEMQTT
#define       MQTT_SHORT_INT  (30*1000)
#define       MQTT_LONG_INT   (5*60*1000)
bool          useMQTT = false;
char          mqttUser[64] = { 0 };
char          mqttPass[64] = { 0 };
char          mqttServer[80] = { 0 };
uint16_t      mqttPort = 1883;
bool          pubMQTT = false;
static unsigned long mqttReconnectNow = 0;
static unsigned long mqttReconnectInt = MQTT_SHORT_INT;
static uint16_t      mqttReconnFails = 0;
static bool          mqttSubAttempted = false;
static bool          mqttOldState = true;
static bool          mqttDoPing = true;
static bool          mqttRestartPing = false;
static bool          mqttPingDone = false;
static unsigned long mqttPingNow = 0;
static unsigned long mqttPingInt = MQTT_SHORT_INT;
static uint16_t      mqttPingsExpired = 0;
#endif

static void wifiConnect(bool deferConfigPortal = false);
static void saveParamsCallback();
static void saveConfigCallback();
static void preUpdateCallback();
static void preSaveConfigCallback();
static void waitConnectCallback();

static void setupStaticIP();
static bool isIp(char *str);
static void ipToString(char *str, IPAddress ip);
static IPAddress stringToIp(char *str);

static void getParam(String name, char *destBuf, size_t length);
static bool myisspace(char mychar);
static char* strcpytrim(char* destination, const char* source, bool doFilter = false);
static void mystrcpy(char *sv, WiFiManagerParameter *el);
#ifndef TC_NOCHECKBOXES
static void strcpyCB(char *sv, WiFiManagerParameter *el);
static void setCBVal(WiFiManagerParameter *el, char *sv);
#endif

#ifdef FC_HAVEMQTT
static void strcpyutf8(char *dst, const char *src, unsigned int len);
static void mqttPing();
static bool mqttReconnect(bool force = false);
static void mqttLooper();
static void mqttCallback(char *topic, byte *payload, unsigned int length);
static void mqttSubscribe();
#endif

/*
 * wifi_setup()
 *
 */
void wifi_setup()
{
    int temp;

    // Explicitly set mode, esp allegedly defaults to STA_AP
    WiFi.mode(WIFI_MODE_STA);

    #ifndef FC_DBG
    wm.setDebugOutput(false);
    #endif

    wm.setParamsPage(true);
    wm.setBreakAfterConfig(true);
    wm.setConfigPortalBlocking(false);
    wm.setPreSaveConfigCallback(preSaveConfigCallback);
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setPreOtaUpdateCallback(preUpdateCallback);
    wm.setHostname(settings.hostName);
    wm.setCaptivePortalEnable(false);
    
    // Our style-overrides, the page title
    wm.setCustomHeadElement(myHead);
    wm.setTitle(F("Flux Capacitor"));
    wm.setDarkMode(false);

    // Hack version number into WiFiManager main page
    wm.setCustomMenuHTML(myCustMenu);

    // Static IP info is not saved by WiFiManager,
    // have to do this "manually". Hence ipsettings.
    wm.setShowStaticFields(true);
    wm.setShowDnsFields(true);

    temp = (int)atoi(settings.wifiConTimeout);
    if(temp < 7) temp = 7;
    if(temp > 25) temp = 25;
    wm.setConnectTimeout(temp);

    temp = (int)atoi(settings.wifiConRetries);
    if(temp < 1) temp = 1;
    if(temp > 15) temp = 15;
    wm.setConnectRetries(temp);

    wm.setCleanConnect(true);
    //wm.setRemoveDuplicateAPs(false);

    wm.setMenu(wifiMenu, TC_MENUSIZE);
    
    wm.addParameter(&custom_sectstart_head);// 3
    wm.addParameter(&custom_playFLUXSnd);
    wm.addParameter(&custom_playTTSnd);

    wm.addParameter(&custom_sectstart);     // 5
    wm.addParameter(&custom_useVknob);
    wm.addParameter(&custom_useSknob);
    wm.addParameter(&custom_TCDpresent);
    wm.addParameter(&custom_swapBL);
    
    wm.addParameter(&custom_sectstart);     // 4
    wm.addParameter(&custom_hostName);
    wm.addParameter(&custom_wifiConRetries);
    wm.addParameter(&custom_wifiConTimeout);    

    #ifdef FC_HAVEMQTT
    wm.addParameter(&custom_sectstart);     // 5
    wm.addParameter(&custom_useMQTT);
    wm.addParameter(&custom_mqttServer);
    wm.addParameter(&custom_mqttUser);
    #endif

    wm.addParameter(&custom_sectstart);     // 3
    wm.addParameter(&custom_musHint);
    wm.addParameter(&custom_shuffle);
    
    wm.addParameter(&custom_sectstart);     // 3
    wm.addParameter(&custom_CfgOnSD);
    //wm.addParameter(&custom_sdFrq);

    #ifdef CP_AUDIO_INSTALLER
    if(check_allow_CPA()) {
        wm.addParameter(&custom_sectstart); // 3
        wm.addParameter(&custom_copyAudio);
        wm.addParameter(&custom_copyHint);
    }
    #endif
    
    wm.addParameter(&custom_sectend_foot);  // 1

    updateConfigPortalValues();

    #ifdef FC_MDNS
    if(MDNS.begin(settings.hostName)) {
        MDNS.addService("http", "tcp", 80);
    }
    #endif

    // No WiFi powersave features here
    wifiOffDelay = 0;
    wifiAPOffDelay = 0;
    
    // Configure static IP
    if(loadIpSettings()) {
        setupStaticIP();
    }

    // Connect, but defer starting the CP
    wifiConnect(true);

#ifdef FC_HAVEMQTT
    useMQTT = ((int)atoi(settings.useMQTT) > 0);
    
    if((!settings.mqttServer[0]) || // No server -> no MQTT
       (wifiInAPMode))              // WiFi in AP mode -> no MQTT
        useMQTT = false;  
    
    if(useMQTT) {

        bool mqttRes = false;
        char *t;
        int tt;

        // No WiFi power save if we're using MQTT
        origWiFiOffDelay = wifiOffDelay = 0;

        if((t = strchr(settings.mqttServer, ':'))) {
            strncpy(mqttServer, settings.mqttServer, t - settings.mqttServer);
            mqttServer[t - settings.mqttServer + 1] = 0;
            tt = atoi(t+1);
            if(tt > 0 && tt <= 65535) {
                mqttPort = tt;
            }
        } else {
            strcpy(mqttServer, settings.mqttServer);
        }

        if(isIp(mqttServer)) {
            mqttClient.setServer(stringToIp(mqttServer), mqttPort);
        } else {
            IPAddress remote_addr;
            if(WiFi.hostByName(mqttServer, remote_addr)) {
                mqttClient.setServer(remote_addr, mqttPort);
            } else {
                mqttClient.setServer(mqttServer, mqttPort);
                // Disable PING if we can't resolve domain
                mqttDoPing = false;
                Serial.printf("MQTT: Failed to resolve '%s'\n", mqttServer);
            }
        }
        
        mqttClient.setCallback(mqttCallback);
        mqttClient.setLooper(mqttLooper);

        if(settings.mqttUser[0] != 0) {
            if((t = strchr(settings.mqttUser, ':'))) {
                strncpy(mqttUser, settings.mqttUser, t - settings.mqttUser);
                mqttUser[t - settings.mqttUser + 1] = 0;
                strcpy(mqttPass, t + 1);
            } else {
                strcpy(mqttUser, settings.mqttUser);
            }
        }

        #ifdef FC_DBG
        Serial.printf("MQTT: server '%s' port %d user '%s' pass '%s'\n", mqttServer, mqttPort, mqttUser, mqttPass);
        #endif
            
        mqttReconnect(true);
        // Rest done in loop
            
    } else {

        #ifdef TC_DBG
        Serial.println("MQTT: Disabled");
        #endif

    }
#endif    
}

/*
 * wifi_loop()
 *
 */
void wifi_loop()
{
    char oldCfgOnSD = 0;
    #ifdef CP_AUDIO_INSTALLER
    bool forceCopyAudio = false;
    #endif

#ifdef FC_HAVEMQTT
    if(useMQTT) {
        if(mqttClient.state() != MQTT_CONNECTING) {
            if(!mqttClient.connected()) {
                if(mqttOldState || mqttRestartPing) {
                    // Disconnection first detected:
                    mqttPingDone = mqttDoPing ? false : true;
                    mqttPingNow = mqttRestartPing ? millis() : 0;
                    mqttOldState = false;
                    mqttRestartPing = false;
                    mqttSubAttempted = false;
                }
                if(mqttDoPing && !mqttPingDone) {
                    audio_loop();
                    mqttPing();
                    audio_loop();
                }
                if(mqttPingDone) {
                    audio_loop();
                    mqttReconnect();
                    audio_loop();
                }
            } else {
                // Only call Subscribe() if connected
                mqttSubscribe();
                mqttOldState = true;
            }
        }
        mqttClient.loop();
    }
#endif    
    
    wm.process();

    #ifdef CP_AUDIO_INSTALLER
    if(shouldSaveConfig > 1) {
        if(check_allow_CPA()) {
            if(!strcmp(custom_copyAudio.getValue(), "FORMAT")) {
                stopAudio();
                formatFlashFS();      // Format
                rewriteSecondarySettings();
                // all others written below
                forceCopyAudio = true;
            }
        }
    }
    #endif
    
    if(shouldSaveIPConfig) {

        #ifdef FC_DBG
        Serial.println(F("WiFi: Saving IP config"));
        #endif

        writeIpSettings();

        shouldSaveIPConfig = false;

    } else if(shouldDeleteIPConfig) {

        #ifdef FC_DBG
        Serial.println(F("WiFi: Deleting IP config"));
        #endif

        deleteIpSettings();

        shouldDeleteIPConfig = false;

    }

    if(shouldSaveConfig) {

        // Save settings and restart esp32

        #ifdef FC_DBG
        Serial.println(F("Config Portal: Saving config"));
        #endif

        // Only read parms if the user actually clicked SAVE on the params page
        if(shouldSaveConfig > 1) {

            strcpytrim(settings.hostName, custom_hostName.getValue(), true);
            if(strlen(settings.hostName) == 0) {
                strcpy(settings.hostName, DEF_HOSTNAME);
            } else {
                char *s = settings.hostName;
                for ( ; *s; ++s) *s = tolower(*s);
            }
            mystrcpy(settings.wifiConRetries, &custom_wifiConRetries);
            mystrcpy(settings.wifiConTimeout, &custom_wifiConTimeout);

            #ifdef FC_HAVEMQTT
            strcpytrim(settings.mqttServer, custom_mqttServer.getValue());
            strcpyutf8(settings.mqttUser, custom_mqttUser.getValue(), sizeof(settings.mqttUser));
            #endif
            
            #ifdef TC_NOCHECKBOXES // --------- Plain text boxes:

            mystrcpy(settings.playFLUXsnd, &custom_playFLUXSnd);
            mystrcpy(settings.playTTsnds, &custom_playTTSnd);
            
            mystrcpy(settings.useVknob, &custom_useVknob);
            mystrcpy(settings.useSknob, &custom_useSknob);

            mystrcpy(settings.TCDpresent, &custom_TCDpresent);
            mystrcpy(settings.usePLforBL, &custom_swapBL);

            #ifdef FC_HAVEMQTT
            mystrcpy(settings.useMQTT, &custom_useMQTT);
            #endif

            mystrcpy(settings.shuffle, &custom_shuffle);

            oldCfgOnSD = settings.CfgOnSD[0];
            mystrcpy(settings.CfgOnSD, &custom_CfgOnSD);
            //mystrcpy(settings.sdFreq, &custom_sdFrq);

            #else // -------------------------- Checkboxes:

            strcpyCB(settings.playFLUXsnd, &custom_playFLUXSnd);
            strcpyCB(settings.playTTsnds, &custom_playTTSnd);
            
            strcpyCB(settings.useVknob, &custom_useVknob);
            strcpyCB(settings.useSknob, &custom_useSknob);

            strcpyCB(settings.TCDpresent, &custom_TCDpresent);
            strcpyCB(settings.usePLforBL, &custom_swapBL);

            #ifdef FC_HAVEMQTT
            strcpyCB(settings.useMQTT, &custom_useMQTT);
            #endif

            strcpyCB(settings.shuffle, &custom_shuffle);
            
            oldCfgOnSD = settings.CfgOnSD[0];
            strcpyCB(settings.CfgOnSD, &custom_CfgOnSD);
            //strcpyCB(settings.sdFreq, &custom_sdFrq);

            #endif  // -------------------------

            // Copy volume/speed/IR settings to other medium if
            // user changed respective option
            if(oldCfgOnSD != settings.CfgOnSD[0]) {
                copySettings();
            }

        }

        // Write settings if requested, or no settings file exists
        if(shouldSaveConfig > 1 || !checkConfigExists()) {
            write_settings();
        }

        #ifdef CP_AUDIO_INSTALLER
        if(shouldSaveConfig > 1) {
            if(check_allow_CPA()) {
                if(forceCopyAudio || (!strcmp(custom_copyAudio.getValue(), "COPY"))) {
                    #ifdef FC_DBG
                    Serial.println(F("Config Portal: Copying audio files...."));
                    #endif
                    pwrNeedFullNow();
                    if(!copy_audio_files()) {
                       delay(3000);
                    }
                    delay(2000);
                }
            }
        }
        #endif
        
        shouldSaveConfig = 0;

        // Reset esp32 to load new settings

        #ifdef FC_DBG
        Serial.println(F("Config Portal: Restarting ESP...."));
        #endif

        Serial.flush();

        esp_restart();
    }

    // WiFi power management
    // If a delay > 0 is configured, WiFi is powered-down after timer has
    // run out. The timer starts when the device is powered-up/boots.
    // There are separate delays for AP mode and STA mode.
    // WiFi can be re-enabled for the configured time by holding '7'
    // on the keypad.
    // NTP requests will re-enable WiFi (in STA mode) for a short
    // while automatically.
    if(wifiInAPMode) {
        // Disable WiFi in AP mode after a configurable delay (if > 0)
        if(wifiAPOffDelay > 0) {
            if(!wifiAPIsOff && (millis() - wifiAPModeNow >= wifiAPOffDelay)) {
                wifiOff();
                wifiAPIsOff = true;
                wifiIsOff = false;
                #ifdef FC_DBG
                Serial.println(F("WiFi (AP-mode) is off. Hold '7' to re-enable."));
                #endif
            }
        }
    } else {
        // Disable WiFi in STA mode after a configurable delay (if > 0)
        if(origWiFiOffDelay > 0) {
            if(!wifiIsOff && (millis() - wifiOnNow >= wifiOffDelay)) {
                wifiOff();
                wifiIsOff = true;
                wifiAPIsOff = false;
                #ifdef FC_DBG
                Serial.println(F("WiFi (STA-mode) is off. Hold '7' to re-enable."));
                #endif
            }
        }
    }

}

static void wifiConnect(bool deferConfigPortal)
{
    // Automatically connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(wm.autoConnect("FC-AP")) {
        #ifdef FC_DBG
        Serial.println(F("WiFi connected"));
        #endif

        // Since WM 2.0.13beta, starting the CP invokes an async
        // WiFi scan. This interferes with network access for a 
        // few seconds after connecting. So, during boot, we start
        // the CP later, to allow a quick NTP update.
        if(!deferConfigPortal) {
            wm.startWebPortal();
        }

        // Allow modem sleep:
        // WIFI_PS_MIN_MODEM is the default, and activated when calling this
        // with "true". When this is enabled, received WiFi data can be
        // delayed for as long as the DTIM period.
        // Since it is the default setting, so no need to call it here.
        //WiFi.setSleep(true);

        // Disable modem sleep, don't want delays accessing the CP or
        // with MQTT.
        WiFi.setSleep(false);

        // Set transmit power to max; we might be connecting as STA after
        // a previous period in AP mode.
        #ifdef FC_DBG
        {
            wifi_power_t power = WiFi.getTxPower();
            Serial.printf("WiFi: Max TX power in STA mode %d\n", power);
        }
        #endif
        WiFi.setTxPower(WIFI_POWER_19_5dBm);

        wifiInAPMode = false;
        wifiIsOff = false;
        wifiOnNow = millis();
        wifiAPIsOff = false;  // Sic! Allows checks like if(wifiAPIsOff || wifiIsOff)

    } else {
        #ifdef FC_DBG
        Serial.println(F("Config portal running in AP-mode"));
        #endif

        {
            #ifdef FC_DBG
            int8_t power;
            esp_wifi_get_max_tx_power(&power);
            Serial.printf("WiFi: Max TX power %d\n", power);
            #endif

            // Try to avoid "burning" the ESP when the WiFi mode
            // is "AP" and the speed/vol knob is fully up by reducing
            // the max. transmit power.
            // The choices are:
            // WIFI_POWER_19_5dBm    = 19.5dBm
            // WIFI_POWER_19dBm      = 19dBm
            // WIFI_POWER_18_5dBm    = 18.5dBm
            // WIFI_POWER_17dBm      = 17dBm
            // WIFI_POWER_15dBm      = 15dBm
            // WIFI_POWER_13dBm      = 13dBm
            // WIFI_POWER_11dBm      = 11dBm
            // WIFI_POWER_8_5dBm     = 8.5dBm
            // WIFI_POWER_7dBm       = 7dBm     <-- proven to avoid the issues
            // WIFI_POWER_5dBm       = 5dBm
            // WIFI_POWER_2dBm       = 2dBm
            // WIFI_POWER_MINUS_1dBm = -1dBm
            WiFi.setTxPower(WIFI_POWER_7dBm);

            #ifdef FC_DBG
            esp_wifi_get_max_tx_power(&power);
            Serial.printf("WiFi: Max TX power set to %d\n", power);
            #endif
        }

        wifiInAPMode = true;
        wifiAPIsOff = false;
        wifiAPModeNow = millis();
        wifiIsOff = false;    // Sic!
    }
}

void wifiOff()
{
    if( (!wifiInAPMode && wifiIsOff) ||
        (wifiInAPMode && wifiAPIsOff) ) {
        return;
    }

    wm.stopWebPortal();
    wm.disconnect();
    WiFi.mode(WIFI_OFF);
}

void wifiOn(unsigned long newDelay, bool alsoInAPMode, bool deferCP)
{
    unsigned long desiredDelay;
    unsigned long Now = millis();

    if(wifiInAPMode && !alsoInAPMode) return;

    if(wifiInAPMode) {
        if(wifiAPOffDelay == 0) return;   // If no delay set, auto-off is disabled
        wifiAPModeNow = Now;              // Otherwise: Restart timer
        if(!wifiAPIsOff) return;
    } else {
        if(origWiFiOffDelay == 0) return; // If no delay set, auto-off is disabled
        desiredDelay = (newDelay > 0) ? newDelay : origWiFiOffDelay;
        if((Now - wifiOnNow >= wifiOffDelay) ||                    // If delay has run out, or
           (wifiOffDelay - (Now - wifiOnNow))  < desiredDelay) {   // new delay exceeds remaining delay:
            wifiOffDelay = desiredDelay;                           // Set new timer delay, and
            wifiOnNow = Now;                                       // restart timer
            #ifdef FC_DBG
            Serial.printf("Restarting WiFi-off timer; delay %d\n", wifiOffDelay);
            #endif
        }
        if(!wifiIsOff) {
            // If WiFi is not off, check if user wanted
            // to start the CP, and do so, if not running
            if(!deferCP) {
                if(!wm.getWebPortalActive()) {
                    wm.startWebPortal();
                }
            }
            return;
        }
    }

    WiFi.mode(WIFI_MODE_STA);

    wifiConnect(deferCP);
}

// Check if WiFi is on; used to determine if a 
// longer interruption due to a re-connect is to
// be expected.
bool wifiIsOn()
{
    if(wifiInAPMode) {
        if(wifiAPOffDelay == 0) return true;
        if(!wifiAPIsOff) return true;
    } else {
        if(origWiFiOffDelay == 0) return true;
        if(!wifiIsOff) return true;
    }
    return false;
}

void wifiStartCP()
{
    if(wifiInAPMode || wifiIsOff)
        return;

    wm.startWebPortal();
}

// This is called when the WiFi config changes, so it has
// nothing to do with our settings here. Despite that,
// we write out our config file so that when the user initially
// configures WiFi, a default settings file exists upon reboot.
// Also, this triggers a reboot, so if the user entered static
// IP data, it becomes active after this reboot.
static void saveConfigCallback()
{
    shouldSaveConfig = 1;
}

// This is the callback from the actual Params page. In this
// case, we really read out the server parms and save them.
static void saveParamsCallback()
{
    shouldSaveConfig = 2;
}

// This is called before a firmware updated is initiated.
// Disable WiFi-off-timers.
static void preUpdateCallback()
{
    wifiAPOffDelay = 0;
    origWiFiOffDelay = 0;

    showWaitSequence();

    stopAudio();
}

// Grab static IP parameters from WiFiManager's server.
// Since there is no public method for this, we steal
// the html form parameters in this callback.
static void preSaveConfigCallback()
{
    char ipBuf[20] = "";
    char gwBuf[20] = "";
    char snBuf[20] = "";
    char dnsBuf[20] = "";
    bool invalConf = false;

    #ifdef FC_DBG
    Serial.println("preSaveConfigCallback");
    #endif

    // clear as strncpy might leave us unterminated
    memset(ipBuf, 0, 20);
    memset(gwBuf, 0, 20);
    memset(snBuf, 0, 20);
    memset(dnsBuf, 0, 20);

    if(wm.server->arg(FPSTR(S_ip)) != "") {
        strncpy(ipBuf, wm.server->arg(FPSTR(S_ip)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_gw)) != "") {
        strncpy(gwBuf, wm.server->arg(FPSTR(S_gw)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_sn)) != "") {
        strncpy(snBuf, wm.server->arg(FPSTR(S_sn)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_dns)) != "") {
        strncpy(dnsBuf, wm.server->arg(FPSTR(S_dns)).c_str(), 19);
    } else invalConf |= true;

    #ifdef FC_DBG
    if(strlen(ipBuf) > 0) {
        Serial.printf("IP:%s / SN:%s / GW:%s / DNS:%s\n", ipBuf, snBuf, gwBuf, dnsBuf);
    } else {
        Serial.println("Static IP unset, using DHCP");
    }
    #endif

    if(!invalConf && isIp(ipBuf) && isIp(gwBuf) && isIp(snBuf) && isIp(dnsBuf)) {

        #ifdef FC_DBG
        Serial.println("All IPs valid");
        #endif

        strcpy(ipsettings.ip, ipBuf);
        strcpy(ipsettings.gateway, gwBuf);
        strcpy(ipsettings.netmask, snBuf);
        strcpy(ipsettings.dns, dnsBuf);

        shouldSaveIPConfig = true;

    } else {

        #ifdef FC_DBG
        if(strlen(ipBuf) > 0) {
            Serial.println("Invalid IP");
        }
        #endif

        shouldDeleteIPConfig = true;

    }
}

static void setupStaticIP()
{
    IPAddress ip;
    IPAddress gw;
    IPAddress sn;
    IPAddress dns;

    if(strlen(ipsettings.ip) > 0 &&
        isIp(ipsettings.ip) &&
        isIp(ipsettings.gateway) &&
        isIp(ipsettings.netmask) &&
        isIp(ipsettings.dns)) {

        ip = stringToIp(ipsettings.ip);
        gw = stringToIp(ipsettings.gateway);
        sn = stringToIp(ipsettings.netmask);
        dns = stringToIp(ipsettings.dns);

        wm.setSTAStaticIPConfig(ip, gw, sn, dns);
    }
}

void updateConfigPortalValues()
{
    const char custHTMLSel[] = " selected";

    // Make sure the settings form has the correct values

    custom_hostName.setValue(settings.hostName, 31);
    custom_wifiConTimeout.setValue(settings.wifiConTimeout, 2);
    custom_wifiConRetries.setValue(settings.wifiConRetries, 2);

    #ifdef FC_HAVEMQTT
    custom_mqttServer.setValue(settings.mqttServer, 79);
    custom_mqttUser.setValue(settings.mqttUser, 63);
    #endif

    #ifdef TC_NOCHECKBOXES  // Standard text boxes: -------

    custom_playFLUXSnd.setValue(settings.playFLUXsnd, 1);
    custom_playTTSnd.setValue(settings.playTTsnds, 1);
    
    custom_useVknob.setValue(settings.useVknob, 1);
    custom_useSknob.setValue(settings.useSknob, 1);

    custom_TCDpresent.setValue(settings.TCDpresent, 1);
    custom_swapBL.setValue(settings.usePLforBL, 1);

    #ifdef FC_HAVEMQTT
    custom_useMQTT.setValue(settings.useMQTT, 1);
    #endif

    custom_shuffle.setValue(settings.shuffle, 1);
    
    custom_CfgOnSD.setValue(settings.CfgOnSD, 1);
    //custom_sdFrq.setValue(settings.sdFreq, 1);
    
    #else   // For checkbox hack --------------------------

    setCBVal(&custom_playFLUXSnd, settings.playFLUXsnd);
    setCBVal(&custom_playTTSnd, settings.playTTsnds);
    
    setCBVal(&custom_useVknob, settings.useVknob);
    setCBVal(&custom_useSknob, settings.useSknob);

    setCBVal(&custom_TCDpresent, settings.TCDpresent);
    setCBVal(&custom_swapBL, settings.usePLforBL);

    #ifdef FC_HAVEMQTT
    setCBVal(&custom_useMQTT, settings.useMQTT);
    #endif

    setCBVal(&custom_shuffle, settings.shuffle);
    
    setCBVal(&custom_CfgOnSD, settings.CfgOnSD);
    //setCBVal(&custom_sdFrq, settings.sdFreq);

    #endif // ---------------------------------------------    

    #ifdef CP_AUDIO_INSTALLER
    custom_copyAudio.setValue("", 6);   // Always clear
    #endif
}

bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d)
{
    IPAddress myip;

    switch(WiFi.getMode()) {
      case WIFI_MODE_STA:
          myip = WiFi.localIP();
          break;
      case WIFI_MODE_AP:
      case WIFI_MODE_APSTA:
          myip = WiFi.softAPIP();
          break;
      default:
          a = b = c = d = 0;
          return true;
    }

    a = myip[0];
    b = myip[1];
    c = myip[2];
    d = myip[3];

    return true;
}

// Check if String is a valid IP address
static bool isIp(char *str)
{
    int segs = 0;
    int digcnt = 0;
    int num = 0;

    while(*str != '\0') {

        if(*str == '.') {

            if(!digcnt || (++segs == 4))
                return false;

            num = digcnt = 0;
            str++;
            continue;

        } else if((*str < '0') || (*str > '9')) {

            return false;

        }

        if((num = (num * 10) + (*str - '0')) > 255)
            return false;

        digcnt++;
        str++;
    }

    if(segs == 3) 
        return true;

    return false;
}

// IPAddress to string
static void ipToString(char *str, IPAddress ip)
{
    sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

// String to IPAddress
static IPAddress stringToIp(char *str)
{
    int ip1, ip2, ip3, ip4;

    sscanf(str, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

    return IPAddress(ip1, ip2, ip3, ip4);
}

/*
 * Read parameter from server, for customhmtl input
 */
static void getParam(String name, char *destBuf, size_t length)
{
    memset(destBuf, 0, length+1);
    if(wm.server->hasArg(name)) {
        strncpy(destBuf, wm.server->arg(name).c_str(), length);
    }
}

static bool myisspace(char mychar)
{
    return (mychar == ' ' || mychar == '\n' || mychar == '\t' || mychar == '\v' || mychar == '\f' || mychar == '\r');
}

static bool myisgoodchar(char mychar)
{
    return ((mychar >= '0' && mychar <= '9') || (mychar >= 'a' && mychar <= 'z') || (mychar >= 'A' && mychar <= 'Z') || mychar == '-');
}

static char* strcpytrim(char* destination, const char* source, bool doFilter)
{
    char *ret = destination;
    
    do {
        if(!myisspace(*source) && (!doFilter || myisgoodchar(*source))) *destination++ = *source;
        source++;
    } while(*source);
    
    *destination = 0;
    
    return ret;
}

static void mystrcpy(char *sv, WiFiManagerParameter *el)
{
    strcpy(sv, el->getValue());
}

#ifndef TC_NOCHECKBOXES
static void strcpyCB(char *sv, WiFiManagerParameter *el)
{
    strcpy(sv, ((int)atoi(el->getValue()) > 0) ? "1" : "0");
}

static void setCBVal(WiFiManagerParameter *el, char *sv)
{
    const char makeCheck[] = "1' checked a='";
    
    el->setValue(((int)atoi(sv) > 0) ? makeCheck : "1", 14);
}
#endif

#ifdef FC_HAVEMQTT
static void strcpyutf8(char *dst, const char *src, unsigned int len)
{
    strncpy(dst, src, len - 1);
    dst[len] = 0;
}

static int16_t filterOutUTF8(char *src, char *dst)
{
    int i, j, slen = strlen(src);
    unsigned char c, d, e, f;

    for(i = 0, j = 0; i < slen; i++) {
        c = (unsigned char)src[i];
        if(c >= 32 && c < 127) {
            if(c >= 'a' && c <= 'z') c &= ~0x20;
            dst[j++] = c;
        } else if(c >= 194 && c < 224 && (i+1) < slen) {
            d = (unsigned char)src[i+1];
            if(d > 127 && d < 192) i++;
        } else if(c < 240 && (i+2) < slen) {
            d = (unsigned char)src[i+1];
            e = (unsigned char)src[i+2];
            if(d > 127 && d < 192 && 
               e > 127 && e < 192) 
                i+=2;
        } else if(c < 245 && (i+3) < slen) {
            d = (unsigned char)src[i+1];
            e = (unsigned char)src[i+2];
            f = (unsigned char)src[i+3];
            if(d > 127 && d < 192 && 
               e > 127 && e < 192 && 
               f > 127 && f < 192)
                i+=3;
        }
    }
    dst[j] = 0;

    return j;
}

static void mqttLooper()
{
    audio_loop();
}

static void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    int i = 0, j, ml = (length <= 255) ? length : 255;
    char tempBuf[256];
    static const char *cmdList[] = {
      "TIMETRAVEL",       // 0
      "FLUX_ON",          // 1
      "FLUX_OFF",         // 2
      "MP_SHUFFLE_ON",    // 3
      "MP_SHUFFLE_OFF",   // 4
      "MP_PLAY",          // 5
      "MP_STOP",          // 6
      "MP_NEXT",          // 7
      "MP_PREV",          // 8
      NULL
    };
    static const char *cmdList2[] = {
      "TIMETRAVEL",       // 0
      "REENTRY",          // 1
      "ALARM",            // 2
      NULL
    };

    if(!length) return;

    memcpy(tempBuf, (const char *)payload, ml);
    tempBuf[ml] = 0;
    for(j = 0; j < ml; j++) {
        if(tempBuf[j] >= 'a' && tempBuf[j] <= 'z') tempBuf[j] &= ~0x20;
    }

    if(!strcmp(topic, "bttf/tcd/pub")) {

        // Commands from TCD or other props
    
        // scan for TIMETRAVEL, trigger it
        while(cmdList2[i]) {
            j = strlen(cmdList2[i]);
            if((length >= j) && !strncmp((const char *)tempBuf, cmdList2[i], j)) {
                break;
            }
            i++;          
        }

        if(!cmdList2[i]) return;

        switch(i) {
        case 0:
            // trigger Time Travel (if not running already)
            if(!TTrunning && !IRLearning) {
                mqttTimeTravel = true;
                mqttTCDTT = true;
                mqttReentry = false;
            }
            break;
        case 1:
            // Start re-entry (if TT currently running)
            if(TTrunning && mqttTCDTT) {
                mqttReentry = true;
            }
            break;
        case 2:
            mqttAlarm = true;
            // Eval this at our convenience
            break;
        }
       
    } else if(!strcmp(topic, "bttf/fc/cmd")) {

        // User commands

        // Not taking commands under these circumstances:
        if(TTrunning || IRLearning)
            return;

        while(cmdList[i]) {
            j = strlen(cmdList[i]);
            if((length >= j) && !strncmp((const char *)tempBuf, cmdList[i], j)) {
                break;
            }
            i++;          
        }

        if(!cmdList[i]) return;

        switch(i) {
        case 0:
            // trigger Time Travel; treated like button, not
            // like TT from TCD.
            mqttTimeTravel = true;
            mqttTCDTT = false;
            break;
        case 1:
            playFLUX = true;
            append_flux();
            break;
        case 2:
            if(playingFlux) {
                stopAudio();
            }
            playFLUX = false;
            break;
        case 3:
        case 4:
            if(haveMusic) mp_makeShuffle((i == 3));
            break;
        case 5:    
            if(haveMusic) mp_play();
            break;
        case 6:
            if(haveMusic && mpActive) {
                mp_stop();
                if(playFLUX) {
                    play_flux();
                }
            }
            break;
        case 7:
            if(haveMusic) mp_next(mpActive);
            break;
        case 8:
            if(haveMusic) mp_prev(mpActive);
            break;
        }
            
    } 
}

#ifdef FC_DBG
#define MQTT_FAILCOUNT 6
#else
#define MQTT_FAILCOUNT 120
#endif

static void mqttPing()
{
    switch(mqttClient.pstate()) {
    case PING_IDLE:
        if(WiFi.status() == WL_CONNECTED) {
            if(!mqttPingNow || (millis() - mqttPingNow > mqttPingInt)) {
                mqttPingNow = millis();
                if(!mqttClient.sendPing()) {
                    // Mostly fails for internal reasons;
                    // skip ping test in that case
                    mqttDoPing = false;
                    mqttPingDone = true;  // allow mqtt-connect attempt
                }
            }
        }
        break;
    case PING_PINGING:
        if(mqttClient.pollPing()) {
            mqttPingDone = true;          // allow mqtt-connect attempt
            mqttPingNow = 0;
            mqttPingsExpired = 0;
            mqttPingInt = MQTT_SHORT_INT; // Overwritten on fail in reconnect
            // Delay re-connection for 5 seconds after first ping echo
            mqttReconnectNow = millis() - (mqttReconnectInt - 5000);
        } else if(millis() - mqttPingNow > 5000) {
            mqttClient.cancelPing();
            mqttPingNow = millis();
            mqttPingsExpired++;
            mqttPingInt = MQTT_SHORT_INT * (1 << (mqttPingsExpired / MQTT_FAILCOUNT));
            mqttReconnFails = 0;
        }
        break;
    } 
}

static bool mqttReconnect(bool force)
{
    bool success = false;

    if(useMQTT && (WiFi.status() == WL_CONNECTED)) {

        if(!mqttClient.connected()) {
    
            if(force || !mqttReconnectNow || (millis() - mqttReconnectNow > mqttReconnectInt)) {

                #ifdef FC_DBG
                Serial.println("MQTT: Attempting to (re)connect");
                #endif
    
                if(strlen(mqttUser)) {
                    success = mqttClient.connect(settings.hostName, mqttUser, strlen(mqttPass) ? mqttPass : NULL);
                } else {
                    success = mqttClient.connect(settings.hostName);
                }
    
                mqttReconnectNow = millis();
                
                if(!success) {
                    mqttRestartPing = true;  // Force PING check before reconnection attempt
                    mqttReconnFails++;
                    if(mqttDoPing) {
                        mqttPingInt = MQTT_SHORT_INT * (1 << (mqttReconnFails / MQTT_FAILCOUNT));
                    } else {
                        mqttReconnectInt = MQTT_SHORT_INT * (1 << (mqttReconnFails / MQTT_FAILCOUNT));
                    }
                    #ifdef FC_DBG
                    Serial.printf("MQTT: Failed to reconnect (%d)\n", mqttReconnFails);
                    #endif
                } else {
                    mqttReconnFails = 0;
                    mqttReconnectInt = MQTT_SHORT_INT;
                    #ifdef FC_DBG
                    Serial.println("MQTT: Connected to broker, waiting for CONNACK");
                    #endif
                }
    
                return success;
            } 
        }
    }
      
    return true;
}

static void mqttSubscribe()
{
    // Meant only to be called when connected!
    if(!mqttSubAttempted) {
        if(!mqttClient.subscribe("bttf/fc/cmd", "bttf/tcd/pub")) {
            #ifdef FC_DBG
            Serial.println("MQTT: Failed to subscribe to command topics");
            #endif
        }
        mqttSubAttempted = true;
    }
}

bool mqttState()
{
    return (useMQTT && mqttClient.connected());
}

void mqttPublish(const char *topic, const char *pl, unsigned int len)
{
    if(useMQTT) {
        mqttClient.publish(topic, (uint8_t *)pl, len, false);
    }
}           

#endif
