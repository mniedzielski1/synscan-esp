// Strona debugera (osobny plik, zeby preprocesor Arduino nie parsowal JavaScriptu)
#pragma once
#include <Arduino.h>
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html lang="pl"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SynScan debug</title>
<style>
body{font-family:system-ui,sans-serif;margin:12px;background:#111;color:#ddd}
h1{font-size:18px;margin:0 0 8px}
.c{display:flex;flex-wrap:wrap;gap:6px;margin:8px 0}
.k{background:#222;padding:6px 10px;border-radius:6px;font-size:13px}
.k b{color:#fff}
button,input,select{font-size:14px;padding:6px 10px;border-radius:6px;border:1px solid #444;background:#222;color:#ddd}
input{width:120px}
table{width:100%;border-collapse:collapse;font-family:ui-monospace,monospace;font-size:12px}
td{padding:3px 6px;border-bottom:1px solid #222;vertical-align:top;word-break:break-all}
.APP{color:#6cf}.MNT{color:#7d7}.TEST{color:#fc6}.BRAK{color:#f66}.INFO{color:#aaa}
small{color:#888}
</style></head><body>
<h1>SynScan WiFi: debuger</h1>
<div class="c" id="cnt"></div>
<div class="c">
<button onclick="snd(':e1')">:e1 wersja</button>
<button onclick="snd(':a1')">:a1 kroki/obrot</button>
<button onclick="snd(':f1')">:f1 status</button>
<button onclick="snd(':j1')">:j1 pozycja</button>
<input id="cmd" placeholder=":e2"><button onclick="snd(document.getElementById('cmd').value)">Wyslij</button>
</div>
<div class="c">
<button id="eb" onclick="echo()">Filtr echa</button>
<button onclick="fetch('/clear')">Wyczysc log</button>
</div>
<div class="c">
<button onclick="fetch('/scan')">Skan predkosci (:e1)</button>
<button id="sb" onclick="fetch('/sniff?on='+(sn?0:1))">Podsluch</button>
<select id="bd" onchange="fetch('/baud?b='+this.value)"><option>2400</option><option>4800</option><option>9600</option><option>19200</option><option>38400</option><option>57600</option><option>115200</option></select>
</div>
<small>APP: komenda z aplikacji. MNT: surowa odpowiedz montazu (z echem). Poprawna odpowiedz zaczyna sie od "=", blad od "!". Sama komenda w MNT bez "=" to echo: montaz nie odpowiada albo odpowiedz ginie.</small>
<table id="log"></table>
<script>
let ef=true,sn=false;
async function snd(c){if(!c)return;await fetch('/send?c='+encodeURIComponent(c));}
async function echo(){await fetch('/echo?on='+(ef?0:1));}
async function upd(){
 try{
  const r=await fetch('/log');const j=await r.json();ef=j.echo;
  document.getElementById('eb').textContent='Filtr echa: '+(ef?'WL':'WYL');
  sn=j.sniff;document.getElementById('sb').textContent='Podsluch: '+(sn?'WL':'WYL');
  const bd=document.getElementById('bd');if(document.activeElement!==bd)bd.value=j.baud;
  document.getElementById('cnt').innerHTML=
   `<span class="k">Komendy z aplikacji <b>${j.app}</b></span>`+
   `<span class="k">Odpowiedzi montazu <b>${j.mnt}</b></span>`+
   `<span class="k">Brak odpowiedzi <b>${j.noreply}</b></span>`+
   `<span class="k">Bajty echa <b>${j.echob}</b></span>`+
   `<span class="k">Testy <b>${j.test}</b></span>`+
   `<span class="k">Bledy ramki <b>${j.err}</b></span>`+
   `<span class="k">Odrzucone odpowiedzi <b>${j.rej}</b></span>`+
   `<span class="k">Predkosc <b>${j.baud}</b>${j.scan?' (skan)':''}</span>`+
   `<span class="k">Klienci WiFi <b>${j.clients}</b></span>`;
  document.getElementById('log').innerHTML=j.log.map(e=>
   `<tr><td>${(e.t/1000).toFixed(2)}</td><td class="${e.d}">${e.d}</td><td>${e.x}</td></tr>`).join('');
 }catch(e){}
}
setInterval(upd,1000);upd();
</script></body></html>)HTML";
