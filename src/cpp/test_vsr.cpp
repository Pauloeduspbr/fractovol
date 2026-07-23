//+------------------------------------------------------------------+
//| test_vsr.cpp — teste de mesa da volume_sr.dll                    |
//|                                                                  |
//| O teste decisivo aqui e o T3: um pivo CONSTRUIDO a mao tem de    |
//| ser encontrado na barra exata, com nivel e zona exatos. Sem ele  |
//| um detector de fractal pode "achar" coisas e parecer certo.      |
//| T4 e T5 fecham o cerco: pivo sem volume NAO conta, e o pivo      |
//| fica 3 barras atras da deteccao (o atraso proprio do fractal).   |
//+------------------------------------------------------------------+
#include <windows.h>
#include <cstdio>
#include <cmath>
#include <vector>

#define RES  1
#define SUP -1

typedef int (__stdcall *fn_ver_t)(void);
typedef int (__stdcall *fn_scan_t)(const double*, const double*, const double*,
                                   const double*, const double*, int, int,
                                   double*, double*, double*, double*, double*, int);

static fn_ver_t  VsrVersion = nullptr;
static fn_scan_t VsrScan    = nullptr;
static int g_fail = 0;

static void check(bool ok, const char *name, const char *detail = "")
{
   std::printf("%-6s %s %s\n", ok ? "[ OK ]" : "[FAIL]", name, detail);
   if(!ok) ++g_fail;
}

//--- serie base plana; os pivos sao inseridos pelos testes
struct Serie
{
   std::vector<double> o,h,l,c,v;
   explicit Serie(int n) : o(n,100.0),h(n,100.5),l(n,99.5),c(n,100.0),v(n,1000.0) {}
   //--- monta um topo em `p`: alturas crescentes ate p, decrescentes depois
   void topo(int p, double pico, double vol)
   {
      h[p-2]=pico-1.0; h[p-1]=pico-0.5; h[p]=pico; h[p+1]=pico-0.6; h[p+2]=pico-1.2;
      for(int k=p-2;k<=p+2;++k){ l[k]=h[k]-1.0; o[k]=h[k]-0.8; c[k]=h[k]-0.2; }
      v[p]=vol;
   }
   //--- monta um fundo em `p`
   void fundo(int p, double vale, double vol)
   {
      l[p-2]=vale+1.0; l[p-1]=vale+0.5; l[p]=vale; l[p+1]=vale+0.6; l[p+2]=vale+1.2;
      for(int k=p-2;k<=p+2;++k){ h[k]=l[k]+1.0; o[k]=l[k]+0.2; c[k]=l[k]+0.8; }
      v[p]=vol;
   }
};

int main()
{
   HMODULE hm = LoadLibraryA("volume_sr.dll");
   if(!hm){ std::printf("[FAIL] LoadLibrary erro %lu\n", GetLastError()); return 1; }
   VsrVersion = (fn_ver_t)(void*)GetProcAddress(hm,"VsrVersion");
   VsrScan    = (fn_scan_t)(void*)GetProcAddress(hm,"VsrScan");
   if(!VsrVersion||!VsrScan){ std::printf("[FAIL] GetProcAddress\n"); return 1; }

   check(VsrVersion()==1, "T1 ABI version == 1");

   const int N = 200, CAP = 50;
   std::vector<double> pb(CAP),fb(CAP),ty(CAP),lv(CAP),zn(CAP);

   //--- T2 fail-fast
   {
      Serie s(N);
      const int r1 = VsrScan(nullptr,s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                             pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      const int r2 = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),5,6,
                             pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      const int r3 = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,0,
                             pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      check(r1==-1&&r2==-1&&r3==-1, "T2 fail-fast em argumento invalido");
   }

   //--- T3 CRITICO: acha o pivo construido, na barra e no nivel exatos
   {
      Serie s(N);
      const int P = 100;
      s.topo(P, 110.0, 5000.0);                 // volume bem acima da media
      const int n = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                            pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      int achou = -1;
      for(int k=0;k<n;++k) if((int)pb[k]==P && (int)ty[k]==RES) achou=k;
      const double zonaEsperada = (s.c[P] >= s.o[P]) ? s.c[P] : s.o[P];
      char d[220];
      if(achou >= 0)
         std::snprintf(d,sizeof(d),"(pivo=%.0f nivel=%.2f (high=%.2f) zona=%.2f (esperada=%.2f))",
                       pb[achou],lv[achou],s.h[P],zn[achou],zonaEsperada);
      else
         std::snprintf(d,sizeof(d),"(NAO encontrou o pivo em %d ; %d zonas no total)",P,n);
      check(achou>=0 && std::fabs(lv[achou]-110.0)<1e-9 &&
            std::fabs(zn[achou]-zonaEsperada)<1e-9,
            "T3 acha o pivo de topo com nivel e zona exatos", d);
   }

   //--- T4 CRITICO: pivo IDENTICO mas sem volume NAO vira zona
   {
      Serie s(N);
      const int P = 100;
      s.topo(P, 110.0, 900.0);                  // volume ABAIXO da media (1000)
      const int n = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                            pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      bool achou=false;
      for(int k=0;k<n;++k) if((int)pb[k]==P) achou=true;
      char d[190];
      std::snprintf(d,sizeof(d),"(volume do pivo=900 < media=1000 -> %s ; %d zonas)",
                    achou?"ACHOU (errado)":"ignorou", n);
      check(!achou, "T4 pivo sem confirmacao de volume e ignorado", d);
   }

   //--- T5 o pivo fica 3 barras ANTES da confirmacao
   {
      Serie s(N);
      const int P = 100;
      s.topo(P, 110.0, 5000.0);
      const int n = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                            pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      int k=-1; for(int j=0;j<n;++j) if((int)pb[j]==P) k=j;
      char d[190];
      if(k>=0) std::snprintf(d,sizeof(d),"(pivo=%.0f confirmado em %.0f ; atraso=%.0f barras)",
                             pb[k],fb[k],fb[k]-pb[k]);
      else     std::snprintf(d,sizeof(d),"(pivo nao encontrado)");
      check(k>=0 && (int)(fb[k]-pb[k])==3, "T5 confirmacao 3 barras apos o pivo", d);
   }

   //--- T6 pivo de fundo vira SUPORTE com nivel no low
   {
      Serie s(N);
      const int P = 100;
      s.fundo(P, 90.0, 5000.0);
      const int n = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                            pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      int k=-1; for(int j=0;j<n;++j) if((int)pb[j]==P && (int)ty[j]==SUP) k=j;
      const double zonaEsperada = (s.c[P] >= s.o[P]) ? s.o[P] : s.c[P];
      char d[220];
      if(k>=0) std::snprintf(d,sizeof(d),"(pivo=%.0f nivel=%.2f (low=%.2f) zona=%.2f (esperada=%.2f))",
                             pb[k],lv[k],s.l[P],zn[k],zonaEsperada);
      else     std::snprintf(d,sizeof(d),"(pivo de fundo nao encontrado ; %d zonas)",n);
      check(k>=0 && std::fabs(lv[k]-90.0)<1e-9 && std::fabs(zn[k]-zonaEsperada)<1e-9,
            "T6 pivo de fundo vira suporte com nivel no low", d);
   }

   //--- T7 a zona fica sempre ENTRE o nivel e o corpo, nunca invertida
   {
      Serie s(N);
      s.topo(60, 108.0, 4000.0);
      s.fundo(100, 92.0, 4000.0);
      s.topo(140, 112.0, 4000.0);
      const int n = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                            pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      bool ok = (n >= 3);
      for(int k=0;k<n;++k)
      {
         if((int)ty[k]==RES && !(lv[k] >= zn[k])) ok=false;   // res: nivel acima
         if((int)ty[k]==SUP && !(lv[k] <= zn[k])) ok=false;   // sup: nivel abaixo
      }
      char d[190];
      std::snprintf(d,sizeof(d),"(%d zonas ; nivel sempre do lado externo)",n);
      check(ok, "T7 geometria da zona coerente", d);
   }

   //--- T8 capacity mantem as MAIS RECENTES
   {
      Serie s(N);
      for(int p = 30; p <= 170; p += 20) s.topo(p, 105.0 + p*0.01, 4000.0);
      double p2[3],f2[3],t2[3],l2[3],z2[3];
      const int n = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                            p2,f2,t2,l2,z2,3);
      const int nTot = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                               pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      const bool ok = (n==3) && (nTot>3) && (p2[2] == pb[nTot-1]);
      char d[190];
      std::snprintf(d,sizeof(d),"(capacity=3 devolveu %d de %d ; ultima %.0f == %.0f)",
                    n,nTot,p2[2],pb[nTot-1]);
      check(ok, "T8 capacity preserva as zonas mais recentes", d);
   }

   //--- T9 sem estado: repeticao identica
   {
      Serie s(N); s.topo(100,110.0,5000.0);
      Serie outra(N); outra.fundo(80,88.0,4000.0);
      double a[10]; double b[10];
      const int n1 = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                             a,nullptr,nullptr,nullptr,nullptr,10);
      VsrScan(outra.o.data(),outra.h.data(),outra.l.data(),outra.c.data(),outra.v.data(),N,6,
              b,nullptr,nullptr,nullptr,nullptr,10);
      const int n2 = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                             b,nullptr,nullptr,nullptr,nullptr,10);
      bool ok = (n1==n2);
      for(int k=0;k<n1 && ok;++k) if(a[k]!=b[k]) ok=false;
      char d[160];
      std::snprintf(d,sizeof(d),"(%d zonas nas duas passadas)",n1);
      check(ok, "T9 sem estado: repeticao identica", d);
   }

   //--- T10 serie sem pivo nenhum devolve zero
   {
      Serie s(N);
      for(int i=0;i<N;++i){ s.h[i]=100.0+i*0.01; s.l[i]=99.0+i*0.01;
                            s.o[i]=99.5+i*0.01; s.c[i]=99.8+i*0.01; }
      const int n = VsrScan(s.o.data(),s.h.data(),s.l.data(),s.c.data(),s.v.data(),N,6,
                            pb.data(),fb.data(),ty.data(),lv.data(),zn.data(),CAP);
      char d[160];
      std::snprintf(d,sizeof(d),"(serie monotona -> %d zonas)",n);
      check(n==0, "T10 serie sem pivo nao inventa zona", d);
   }

   FreeLibrary(hm);
   std::printf("\n%s  (%d falha(s))\n", g_fail==0?"TODOS OS TESTES PASSARAM":"TESTES FALHARAM", g_fail);
   return g_fail==0?0:1;
}
