//+------------------------------------------------------------------+
//| volume_sr.cpp                                                    |
//| Porte C++ do "Volume-based Support & Resistance Zones V2"        |
//| (c) tommyf100 / synapticex / Lij_MC — Pine v5                     |
//|                                                                  |
//| Mapa Pine -> C++ (linhas do source.pine):                        |
//|   258-259  f_tfUp             -> teste de pivo de topo           |
//|   260-261  f_tfDown           -> teste de pivo de fundo          |
//|   366      ta.sma(vol, n)     -> media movel do volume           |
//|   375-383  CalcFractalUp/Down -> nivel da zona                   |
//|   390-405  CalcFractalUp/Down Zone -> borda do corpo             |
//+------------------------------------------------------------------+
#include "volume_sr.h"
#include <vector>
#include <deque>

namespace {

//--- ta.sma(volume, n): NaN enquanto nao ha n barras
void sma(const double *src, int n, int len, std::vector<double> &out)
{
   out.assign(n, 0.0);
   if(len < 1) return;
   double acc = 0.0;
   for(int i = 0; i < n; ++i)
   {
      acc += src[i];
      if(i >= len) acc -= src[i-len];
      out[i] = (i >= len-1) ? acc/len : -1.0;   // -1 marca "ainda sem media"
   }
}

struct Zone
{
   int    pivotBar;
   int    foundBar;
   int    type;
   double level;
   double zone;
};

} // namespace

//+------------------------------------------------------------------+
int __stdcall VsrVersion(void) { return VSR_ABI_VERSION; }

//+------------------------------------------------------------------+
int __stdcall VsrScan(
   const double *open, const double *high, const double *low,
   const double *close, const double *volume, int size,
   int volMaPeriod,
   double *outPivotBar, double *outFoundBar, double *outType,
   double *outLevel, double *outZone, int capacity)
{
   if(open==nullptr || high==nullptr || low==nullptr ||
      close==nullptr || volume==nullptr)          return -1;
   if(size < 10)                                  return -1;
   if(volMaPeriod < 1 || volMaPeriod > 1000)      return -1;
   if(capacity < 1)                               return -1;

   std::vector<double> volMa;
   sma(volume, size, volMaPeriod, volMa);

   //--- guarda as `capacity` zonas mais RECENTES, descartando as antigas
   //    pela frente (equivale ao array.shift do Pine, linhas 284/291)
   std::deque<Zone> found;

   //--- Pine 258-261. Na barra i o pivo e a barra i-3, e as
   //    referencias [1]..[5] viram i-1..i-5.
   for(int i = 5; i < size; ++i)
   {
      const int p = i - 3;                    // barra do pivo
      if(volMa[p] < 0.0) continue;            // media de volume ainda imatura
      const bool volOk = volume[p] > volMa[p];
      if(!volOk) continue;

      //--- pivo de topo -> resistencia
      if(high[p] > high[i-4] && high[i-4] > high[i-5] &&
         high[i-2] < high[p] && high[i-1] < high[i-2])
      {
         Zone z;
         z.pivotBar = p;
         z.foundBar = i;
         z.type     = VSR_RESISTANCE;
         z.level    = high[p];
         //--- Pine 392: corpo da vela, o topo dele
         z.zone     = (close[p] >= open[p]) ? close[p] : open[p];
         found.push_back(z);
         if((int)found.size() > capacity) found.pop_front();
      }

      //--- pivo de fundo -> suporte
      if(low[p] < low[i-4] && low[i-4] < low[i-5] &&
         low[i-2] > low[p] && low[i-1] > low[i-2])
      {
         Zone z;
         z.pivotBar = p;
         z.foundBar = i;
         z.type     = VSR_SUPPORT;
         z.level    = low[p];
         //--- Pine 401: corpo da vela, a base dele
         z.zone     = (close[p] >= open[p]) ? open[p] : close[p];
         found.push_back(z);
         if((int)found.size() > capacity) found.pop_front();
      }
   }

   const int n = (int)found.size();
   for(int k = 0; k < n; ++k)
   {
      const Zone &z = found[k];
      if(outPivotBar) outPivotBar[k] = (double)z.pivotBar;
      if(outFoundBar) outFoundBar[k] = (double)z.foundBar;
      if(outType)     outType[k]     = (double)z.type;
      if(outLevel)    outLevel[k]    = z.level;
      if(outZone)     outZone[k]     = z.zone;
   }
   return n;
}
