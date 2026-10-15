// Obudowa: NodeMCU-32S (goldpiny w dol, przewody lutowane) + Mini360 + gniazdo RJ45 8p8c (PCB)
// Wymiary w mm. Zmierz swoje elementy i popraw parametry ponizej.
// part = "base" | "lid" | "assembly"
part = "base";  // base | lid | assembly
$fn = 40;

// ---------- OBUDOWA ----------
wall    = 2.2;
floor_t = 1.6;
lid_t   = 2.0;
in_L    = 80;     // wnetrze X
in_W    = 48;     // wnetrze Y
in_H    = 16.5;     // wnetrze Z
tol     = 0.3;    // luz montazowy

// Sruby M3: tulejki w rogach
boss_r      = 4.0;
boss_off    = 3.5;   // srodek sruby od wewnetrznych scian
screw_pilot = 2.7;   // M3 wkrecane w PLA/PETG. Pod wtopke M3: 4.0
screw_depth = 12;
screw_clear = 3.4;   // otwor w pokrywie

// ---------- NodeMCU ESP32 ----------
esp_l   = 48.3;   // dlugosc PCB (Amica 48.3, NodeMCU-32S 48.0)
esp_w   = 26.0;   // szerokosc PCB (NodeMCU ESP8266 v2 Amica; ESP32 NodeMCU-32S = 25.5)
esp_pcb = 1.6;
esp_x   = 1.0;    // odstep PCB od lewej sciany (strona USB)
esp_y   = (in_W - esp_w) / 2;
esp_z   = 9.8;    // spod PCB nad podloga: goldpiny 9 mm + 0.8 luzu (przewody lutowane z boku pinow)
mod_h   = 3.2;    // wysokosc modulu WROOM nad PCB
mod_c   = 15.8;   // srodek ekranu modulu od konca z antena
hold_gap = -0.4; // docisk modulu: ujemny = wcisk (pokrywa lekko sie ugina i dociska plytke)
usb_zc  = esp_z + esp_pcb + 1.4; // srodek gniazda micro USB
usb_hole = [8.6, 4.2];   // przelot na metalowa czesc wtyku
usb_rec  = [11.6, 6.6, 1.0]; // wnęka od zewnatrz na plastik wtyku (szer, wys, glebokosc)

// ---------- Mini360 ----------
mini_l   = 17.2;
mini_w   = 11.2;
mini_pcb = 1.0;
mini_x   = 53;
mini_y   = 31;
mini_z   = 8.0;   // podest (dlugie, sprezyste zatrzaski)

// ---------- RJ45 (gniazdo na PCB) ----------
rj_w      = 16.0;  // szerokosc korpusu
rj_h      = 13.0;  // wysokosc korpusu
rj_d      = 19.0;  // glebokosc korpusu
rj_rail   = 2.8;   // podniesienie (nozki 2.5 mm pod spodem)
rj_y      = 9.5;
rj_open_w = 14.2;  // otwor w scianie (mniejszy niz korpus = kolnierz)
rj_open_h = 11.2;
rj_gap    = -0.3;  // docisk RJ45 przez pokrywe (ujemny = wcisk)

// ========================================================
module rbox(s, r=1){ // prostopadloscian z zaokraglonymi pionowymi krawedziami
  hull() for (x=[r, s[0]-r], y=[r, s[1]-r]) translate([x,y,0]) cylinder(r=r, h=s[2]);
}
boss_pos = [[boss_off, boss_off], [in_L-boss_off, boss_off],
            [boss_off, in_W-boss_off], [in_L-boss_off, in_W-boss_off]];

// ---------- PODSTAWA ----------
module shell(){
  difference(){
    translate([-wall,-wall,-floor_t]) rbox([in_L+2*wall, in_W+2*wall, in_H+floor_t], 3);
    translate([0,0,0]) rbox([in_L, in_W, in_H+1], 1);
  }
  for (p = boss_pos) translate([p[0],p[1],0]) cylinder(r=boss_r, h=in_H);
  // wypelnienie miedzy tulejka a naroznikiem
  for (p = boss_pos) translate([p[0] < in_L/2 ? 0 : p[0], p[1] < in_W/2 ? 0 : p[1], 0])
    cube([boss_off, boss_off, in_H]);
}

module esp_holder(){
  // filar podporowy pod srodkiem plytki (miedzy rzedami pinow)
  translate([esp_x+6, in_W/2-5, 0]) cube([esp_l-12, 10, esp_z]);
  // slupki ustalajace w 4 rogach plytki
  ph = esp_z + esp_pcb + 2.0;
  for (sx=[0,1], sy=[0,1]) {
    x0 = sx==0 ? esp_x - tol - 1.6 : esp_x + esp_l + tol;
    y0 = sy==0 ? esp_y - tol - 1.6 : esp_y + esp_w + tol;
    xl = sx==0 ? x0 : x0 - 3.4;
    yl = sy==0 ? y0 : y0 - 3.4;
    translate([x0, yl, 0]) cube([1.6, 5, ph]);
    translate([xl, y0, 0]) cube([5, 1.6, ph]);
  }
}

module snap_arm(len, h, lip=0.8){ // sprezysty zatrzask, zab skierowany do +Y
  cube([len, 1.0, h]);
  // zab: plaski spod (trzyma PCB), skos od gory (wcisk)
  translate([0, 1.0, h-1.4]) hull(){
    cube([len, lip, 0.01]);
    translate([0,0,1.39]) cube([len, 0.01, 0.01]);
  }
}

module mini_holder(){
  // podest
  translate([mini_x+2, mini_y+2, 0]) cube([mini_l-4, mini_w-4, mini_z]);
  // ograniczniki boczne (dluzsze boki)
  sh = mini_z + mini_pcb + 1.0;
  for (yy=[mini_y - tol - 1.2, mini_y + mini_w + tol])
    translate([mini_x+4, yy, 0]) cube([mini_l-8, 1.2, sh]);
  // zatrzaski na krotszych bokach (srodek, z dala od padow)
  ah = mini_z + mini_pcb + 0.1 + 1.4;
  translate([mini_x - tol, mini_y + mini_w/2 - 2.5, 0]) rotate([0,0,-90]) translate([-5,-1,0]) snap_arm(5, ah);
  translate([mini_x + mini_l + tol, mini_y + mini_w/2 + 2.5, 0]) rotate([0,0,90]) translate([-5,-1,0]) snap_arm(5, ah);
}

module rj_holder(){
  x0 = in_L - rj_d;
  wh = rj_rail + rj_h*0.6;
  // szyny pod korpusem
  for (yy=[rj_y, rj_y+rj_w-2]) translate([x0, yy, 0]) cube([rj_d, 2, rj_rail]);
  // scianki boczne
  for (yy=[rj_y - tol - 1.6, rj_y + rj_w + tol]) translate([x0, yy, 0]) cube([rj_d, 1.6, wh]);
  // tylne ograniczniki (srodek wolny na przewody)
  for (yy=[rj_y - tol - 1.6, rj_y + rj_w - 4 + tol])
    translate([x0 - tol - 1.6, yy, 0]) cube([1.6, 5.6, wh]);
}

module base(){
  difference(){
    union(){ shell(); esp_holder(); mini_holder(); rj_holder(); }
    // otwory pod sruby
    for (p = boss_pos) translate([p[0],p[1],in_H-screw_depth]) cylinder(d=screw_pilot, h=screw_depth+1);
    // otwor RJ45 (prawa sciana)
    translate([in_L-1, rj_y+rj_w/2-rj_open_w/2, rj_rail+rj_h/2-rj_open_h/2]) cube([wall+2, rj_open_w, rj_open_h]);
    // otwor micro USB (lewa sciana): przelot + wneka na wtyk od zewnatrz
    translate([-wall-1, esp_y+esp_w/2-usb_hole[0]/2, usb_zc-usb_hole[1]/2]) cube([wall+2, usb_hole[0], usb_hole[1]]);
    translate([-wall-1, esp_y+esp_w/2-usb_rec[0]/2, usb_zc-usb_rec[1]/2]) cube([usb_rec[2]+1, usb_rec[0], usb_rec[1]]);
    // wentylacja w dluzszych scianach
    for (x=[8:6:46], yy=[-wall-1, in_W-1])
      translate([x, yy, 3]) rbox([2.5, wall+2, 9], 1.2);
  }
}

// ---------- POKRYWA ----------
module lid(){
  difference(){
    union(){
      translate([-wall,-wall,in_H]) rbox([in_L+2*wall, in_W+2*wall, lid_t], 3);
      // kolnierz centrujacy
      difference(){
        translate([tol, tol, in_H-2]) cube([in_L-2*tol, in_W-2*tol, 2]);
        translate([tol+1.2, tol+1.2, in_H-3]) cube([in_L-2*tol-2.4, in_W-2*tol-2.4, 4]);
        for (p = boss_pos) translate([p[0],p[1],in_H-3]) cylinder(r=boss_r+tol+0.3, h=4);
        for (p = boss_pos) translate([p[0] < in_L/2 ? -1 : p[0]-1, p[1] < in_W/2 ? -1 : p[1]-1, in_H-3])
          cube([boss_off+2, boss_off+2, 4]);
        // wyciecie kolnierza nad gniazdem RJ45
        translate([in_L-4, rj_y-tol-2, in_H-3]) cube([5, rj_w+2*tol+4, 4]);
      }
      // docisk modulu ESP32
      mz = esp_z + esp_pcb + mod_h + hold_gap;
      translate([esp_x+esp_l-mod_c, in_W/2, mz]) cylinder(d=8, h=in_H-mz+0.1);
      // docisk RJ45
      rz = rj_rail + rj_h + rj_gap;
      translate([in_L-rj_d/2, rj_y+rj_w/2, rz]) cylinder(d=7, h=in_H-rz+0.1);
    }
    for (p = boss_pos) translate([p[0],p[1],in_H-5]) cylinder(d=screw_clear, h=20);
    // szczeliny wentylacyjne
    for (x=[6:5:24]) translate([x, in_W/2-9, in_H-1]) rbox([2.2, 18, lid_t+2], 1.1);
    for (y=[mini_y-3.5, mini_y+0.5, mini_y+8]) translate([mini_x-2, y, in_H-1]) rbox([20, 2.2, lid_t+2], 1.1);
  }
}

// ---------- ATRAPY do podgladu ----------
module dummies(){
  color("royalblue") translate([esp_x, esp_y, esp_z]) cube([esp_l, esp_w, esp_pcb]);
  color("silver") translate([esp_x+esp_l-25.5, in_W/2-9, esp_z+esp_pcb]) cube([25.5, 18, mod_h]);
  color("gold") for (yy=[esp_y+1.0, esp_y+esp_w-1.6], i=[0:17])
    translate([esp_x+2+i*2.54, yy, esp_z-9.7]) cube([0.64,0.64,9.7]);
  color("green") translate([mini_x, mini_y, mini_z]) cube([mini_l, mini_w, mini_pcb]);
  color("dimgray") translate([in_L-rj_d, rj_y, rj_rail]) cube([rj_d, rj_w, rj_h]);
}

if (part == "base") base();
else if (part == "lid") translate([0,0,in_H+lid_t]) rotate([180,0,0]) lid();
else { base(); dummies(); %lid(); }
