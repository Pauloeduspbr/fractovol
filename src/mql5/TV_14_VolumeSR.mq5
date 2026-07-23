//+------------------------------------------------------------------+
//|                                       TV_14_VolumeSR.mq5         |
//|   Porte MQL5 do "Volume-based Support & Resistance Zones V2"     |
//|   Pine v5 original: (c) tommyf100, sobre trabalho de synapticex  |
//|   e Lij_MC                                                       |
//|                                                                  |
//|   LICENCA: o script original NAO declara licenca explicita.      |
//|   Publicado aberto no TradingView com creditos aos autores.      |
//|   Tratar como uso pessoal/estudo; NAO redistribuir nem usar       |
//|   comercialmente sem confirmar com o autor.                      |
//|                                                                  |
//| Marca zonas de suporte/resistencia em pivos de 5 barras          |
//| CONFIRMADOS POR VOLUME, em ate 4 timeframes ao mesmo tempo.      |
//| Motor em volume_sr.dll (C++); contrato em cpp/14_volume_sr.      |
//|                                                                  |
//| POR QUE OBJETOS E NAO BUFFERS: cada zona e um retangulo que      |
//| comeca no seu proprio pivo e se estende a direita, e ha ate 4    |
//| timeframes sobrepostos. Buffers de indicador guardam um valor    |
//| POR BARRA do grafico atual — nao representam retangulos com      |
//| inicio proprio nem dados de outro periodo. Por isso aqui o       |
//| desenho e com OBJ_RECTANGLE, como o Pine faz com box/linefill.   |
//|                                                                  |
//| Requer "Permitir importacoes de DLL" HABILITADO.                 |
//+------------------------------------------------------------------+
#property copyright "Porte C++/MQL5 — original (c) tommyf100 / synapticex / Lij_MC"
#property link      "https://www.tradingview.com/"
#property version   "1.00"
#property description "Zonas de S/R em pivos confirmados por volume, ate 4 timeframes"
#property description "Motor em volume_sr.dll. Requer simbolo COM dados de volume."

#property indicator_chart_window
#property indicator_buffers 0
#property indicator_plots   0

#define EXPECTED_ABI_VERSION 1
#define OBJ_PREFIX "TV14_VSR_"
#define NTF 4

#import "volume_sr.dll"
int VsrVersion(void);
int VsrScan(const double &open[], const double &high[], const double &low[],
            const double &close[], const double &volume[], int size,
            int volMaPeriod,
            double &outPivotBar[], double &outFoundBar[], double &outType[],
            double &outLevel[], double &outZone[], int capacity);
#import

//--- ATENCAO: nada de `input group` (desloca os parametros de iCustom)
input ENUM_TIMEFRAMES InpTF1      = PERIOD_CURRENT; // [TF1] Timeframe
input int             InpVolMA1   = 6;              // [TF1] Volume MA - Threshold
input int             InpZones1   = 12;             // [TF1] Zonas (0 = desliga)
input color           InpRes1     = clrCrimson;     // [TF1] Cor da resistencia
input color           InpSup1     = clrLimeGreen;   // [TF1] Cor do suporte
input ENUM_TIMEFRAMES InpTF2      = PERIOD_H4;      // [TF2] Timeframe
input int             InpVolMA2   = 6;              // [TF2] Volume MA - Threshold
input int             InpZones2   = 8;              // [TF2] Zonas (0 = desliga)
input color           InpRes2     = clrMagenta;     // [TF2] Cor da resistencia
input color           InpSup2     = clrGreen;       // [TF2] Cor do suporte
input ENUM_TIMEFRAMES InpTF3      = PERIOD_D1;      // [TF3] Timeframe
input int             InpVolMA3   = 6;              // [TF3] Volume MA - Threshold
input int             InpZones3   = 5;              // [TF3] Zonas (0 = desliga)
input color           InpRes3     = clrOrange;      // [TF3] Cor da resistencia
input color           InpSup3     = clrDodgerBlue;  // [TF3] Cor do suporte
input ENUM_TIMEFRAMES InpTF4      = PERIOD_W1;      // [TF4] Timeframe
input int             InpVolMA4   = 6;              // [TF4] Volume MA - Threshold
input int             InpZones4   = 3;              // [TF4] Zonas (0 = desliga)
input color           InpRes4     = clrMaroon;      // [TF4] Cor da resistencia
input color           InpSup4     = clrTeal;        // [TF4] Cor do suporte
input int             InpBarsTF   = 1500;           // Barras lidas de cada timeframe
input bool            InpExtActive= true;           // Estender a zona ATIVA ate a direita
input bool            InpFill     = true;           // Preencher as zonas
input int             InpOpacity  = 14;             // Opacidade do preenchimento (%)
input bool            InpShowLbl  = true;           // Rotulo do timeframe
input bool            InpHideLower= true;           // Ocultar TF menor que o do grafico

datetime g_lastBar = 0;
//--- "geracao" do redesenho. Todo objeto tocado numa passada recebe a
//    geracao atual no ZORDER; no fim apagamos so os que NAO foram
//    tocados. Antes usavamos ObjectsDeleteAll + recriar tudo, o que
//    fazia o grafico PISCAR a cada recalculo.
long g_gen = 0;

//--- marca o objeto como vivo nesta passada
void Tocar(const string nome)
{
   ObjectSetInteger(0, nome, OBJPROP_ZORDER, g_gen);
}

//--- reaproveita as zonas ja desenhadas de um slot quando os dados
//    daquele timeframe ainda nao chegaram (CopyRates incompleto).
//    Sem isso as zonas somem e voltam enquanto o MT5 sincroniza.
void PreservarSlot(const int slot)
{
   const string pref = OBJ_PREFIX + "T" + IntegerToString(slot) + "_";
   for(int i = ObjectsTotal(0,-1,-1)-1; i >= 0; --i)
   {
      const string nm = ObjectName(0,i);
      if(StringFind(nm, pref) == 0) Tocar(nm);
   }
}

//+------------------------------------------------------------------+
//| MQL5 nao tem canal alfa em OBJ_RECTANGLE. Simulamos a             |
//| transparencia do Pine misturando a cor com o preto do fundo.      |
//+------------------------------------------------------------------+
color Fade(const color c, const int pct)
{
   const double k = pct / 100.0;
   const int r=(int)((c&0xFF)*k), g=(int)(((c>>8)&0xFF)*k), b=(int)(((c>>16)&0xFF)*k);
   return (color)(r | (g<<8) | (b<<16));
}

//+------------------------------------------------------------------+
int OnInit()
{
   const int v = VsrVersion();
   if(v != EXPECTED_ABI_VERSION)
   {
      PrintFormat("[TV_14_VSR] ABI incompativel: volume_sr.dll=%d, esperado=%d. "
                  "Recompile (cpp/14_volume_sr/build.sh).", v, EXPECTED_ABI_VERSION);
      return INIT_FAILED;
   }
   if(InpBarsTF < 100 || InpBarsTF > 20000)
   { Print("[TV_14_VSR] Barras por timeframe deve estar em [100,20000]."); return INIT_PARAMETERS_INCORRECT; }
   if(InpOpacity < 1 || InpOpacity > 100)
   { Print("[TV_14_VSR] Opacidade deve estar em [1,100]."); return INIT_PARAMETERS_INCORRECT; }

   //--- o indicador depende de volume real; avisa se o simbolo nao tem
   long vol[];
   ArraySetAsSeries(vol,false);
   if(CopyTickVolume(_Symbol,_Period,0,10,vol) > 0)
   {
      long soma = 0;
      for(int i=0;i<ArraySize(vol);++i) soma += vol[i];
      if(soma <= 0)
         PrintFormat("[TV_14_VSR] %s nao parece ter dados de volume. O indicador "
                     "depende de volume para confirmar os pivos e nao vai marcar "
                     "zona nenhuma.", _Symbol);
   }

   g_lastBar = 0;
   IndicatorSetString(INDICATOR_SHORTNAME, "Vol S/R Zones");
   return INIT_SUCCEEDED;
}

//+------------------------------------------------------------------+
void OnDeinit(const int reason)
{
   ObjectsDeleteAll(0, OBJ_PREFIX);
   ChartRedraw();
}

//+------------------------------------------------------------------+
//| Desenha as zonas de UM timeframe                                  |
//+------------------------------------------------------------------+
int DesenhaTF(const int slot, const ENUM_TIMEFRAMES tf, const int volMa,
              const int nZones, const color corRes, const color corSup,
              const datetime fimGrafico)
{
   if(nZones <= 0) return 0;

   const ENUM_TIMEFRAMES real = (tf == PERIOD_CURRENT) ? (ENUM_TIMEFRAMES)_Period : tf;

   //--- Pine 187-190: zona de timeframe MENOR que o do grafico polui e
   //    nao faz sentido; o original tambem as esconde.
   if(InpHideLower && PeriodSeconds(real) < PeriodSeconds((ENUM_TIMEFRAMES)_Period))
      return 0;

   MqlRates r[];
   ArraySetAsSeries(r, false);
   const int n = CopyRates(_Symbol, real, 0, InpBarsTF, r);
   if(n < 20)
   {
      //--- dados desse timeframe ainda carregando: mantem o que ja esta
      //    desenhado em vez de deixar sumir
      PreservarSlot(slot);
      return 0;
   }

   double o[],h[],l[],c[],v[];
   ArrayResize(o,n); ArrayResize(h,n); ArrayResize(l,n);
   ArrayResize(c,n); ArrayResize(v,n);
   ArraySetAsSeries(o,false); ArraySetAsSeries(h,false); ArraySetAsSeries(l,false);
   ArraySetAsSeries(c,false); ArraySetAsSeries(v,false);
   for(int i = 0; i < n; ++i)
   {
      o[i]=r[i].open; h[i]=r[i].high; l[i]=r[i].low; c[i]=r[i].close;
      v[i]=(double)r[i].tick_volume;
   }

   const int cap = nZones * 2;      // resistencias + suportes
   double pb[],fb[],ty[],lv[],zn[];
   ArrayResize(pb,cap); ArrayResize(fb,cap); ArrayResize(ty,cap);
   ArrayResize(lv,cap); ArrayResize(zn,cap);
   ArraySetAsSeries(pb,false); ArraySetAsSeries(fb,false); ArraySetAsSeries(ty,false);
   ArraySetAsSeries(lv,false); ArraySetAsSeries(zn,false);

   const int found = VsrScan(o,h,l,c,v,n,volMa,pb,fb,ty,lv,zn,cap);
   if(found <= 0) { PreservarSlot(slot); return 0; }

   const string tfTxt = StringSubstr(EnumToString(real), 7);   // PERIOD_H4 -> H4

   //--- ENCADEAMENTO (Pine: input 'Extend all S/R Zones to Next Zone',
   //    default true; linhas 281-283 e 288-290). Cada zona termina onde
   //    a PROXIMA DO MESMO TIPO comeca — nao no fim do grafico. Apenas a
   //    mais recente de cada tipo fica ativa e se estende a direita
   //    ('Extend active S/R Zones to Right'). Sem isso todas as faixas
   //    atravessam a tela inteira e o grafico vira uma parede de cor.
   int ultimoRes = -1, ultimoSup = -1;
   for(int k = 0; k < found; ++k)
   {
      if(ty[k] > 0) ultimoRes = k; else ultimoSup = k;
   }

   int desenhadas = 0;
   for(int k = 0; k < found; ++k)
   {
      const int pi = (int)pb[k];
      if(pi < 0 || pi >= n) continue;

      const bool res = (ty[k] > 0);
      const color cor = res ? corRes : corSup;
      const datetime t1 = r[pi].time;

      //--- fim da zona: inicio da proxima do MESMO tipo
      datetime t2 = fimGrafico;
      bool ativa = true;
      for(int j = k+1; j < found; ++j)
      {
         if((ty[j] > 0) != res) continue;              // outro tipo, ignora
         const int pj = (int)pb[j];
         if(pj < 0 || pj >= n) continue;
         t2 = r[pj].time;
         ativa = false;
         break;
      }
      //--- a zona ativa so vai ate a direita se o usuario quiser
      if(ativa && !InpExtActive)
         t2 = r[n-1].time;

      if(t2 <= t1) continue;                            // zona degenerada

      const string id = StringFormat("%sT%d_%d_%s", OBJ_PREFIX, slot, k, res ? "R" : "S");

      //--- a zona vai do nivel ate a borda do corpo da vela do pivo
      const double y1 = lv[k], y2 = zn[k];

      if(ObjectFind(0, id) < 0)
      {
         ObjectCreate(0, id, OBJ_RECTANGLE, 0, t1, y1, t2, y2);
         ObjectSetInteger(0, id, OBJPROP_SELECTABLE, false);
         ObjectSetInteger(0, id, OBJPROP_BACK, true);
      }
      ObjectMove(0, id, 0, t1, y1);
      ObjectMove(0, id, 1, t2, y2);
      ObjectSetInteger(0, id, OBJPROP_FILL, InpFill);
      ObjectSetInteger(0, id, OBJPROP_COLOR, InpFill ? Fade(cor, InpOpacity) : cor);
      ObjectSetInteger(0, id, OBJPROP_WIDTH, 1);
      Tocar(id);

      //--- borda no nivel (o lado que realmente serve de S/R)
      const string idl = id + "L";
      if(ObjectFind(0, idl) < 0)
      {
         ObjectCreate(0, idl, OBJ_TREND, 0, t1, y1, t2, y1);
         ObjectSetInteger(0, idl, OBJPROP_RAY_RIGHT, false);
         ObjectSetInteger(0, idl, OBJPROP_SELECTABLE, false);
         ObjectSetInteger(0, idl, OBJPROP_BACK, true);
      }
      ObjectMove(0, idl, 0, t1, y1);
      ObjectMove(0, idl, 1, t2, y1);
      ObjectSetInteger(0, idl, OBJPROP_COLOR, cor);
      Tocar(idl);

      //--- rotulo so na zona ATIVA de cada tipo (Pine 416-417)
      if(InpShowLbl && (k == ultimoRes || k == ultimoSup))
      {
         const string idt = id + "T";
         if(ObjectFind(0, idt) < 0)
         {
            ObjectCreate(0, idt, OBJ_TEXT, 0, t2, y1);
            ObjectSetInteger(0, idt, OBJPROP_FONTSIZE, 8);
            ObjectSetInteger(0, idt, OBJPROP_ANCHOR, ANCHOR_RIGHT);
            ObjectSetInteger(0, idt, OBJPROP_SELECTABLE, false);
         }
         ObjectMove(0, idt, 0, t2, y1);
         ObjectSetInteger(0, idt, OBJPROP_COLOR, cor);
         ObjectSetString(0, idt, OBJPROP_TEXT, tfTxt + (res ? " (R)" : " (S)"));
         Tocar(idt);
      }
      ++desenhadas;
   }
   return desenhadas;
}

//+------------------------------------------------------------------+
int OnCalculate(const int rates_total,
                const int prev_calculated,
                const datetime &time[],
                const double &open[],
                const double &high[],
                const double &low[],
                const double &close[],
                const long &tick_volume[],
                const long &volume[],
                const int &spread[])
{
   if(rates_total < 50) return 0;

   //--- so redesenha em barra NOVA. O teste antigo dependia de
   //    prev_calculated>0, e o MT5 zera esse valor em varias situacoes
   //    (mudanca de escala, chegada de historico), o que disparava um
   //    redesenho completo — outra fonte do piscar.
   static bool primeira = true;
   if(!primeira && g_lastBar == time[rates_total-1]) return rates_total;
   primeira  = false;
   g_lastBar = time[rates_total-1];
   ++g_gen;

   //--- as zonas se estendem ate o fim do grafico visivel
   const datetime fim = time[rates_total-1] + (datetime)PeriodSeconds(_Period)*20;

   int total = 0;
   total += DesenhaTF(1, InpTF1, InpVolMA1, InpZones1, InpRes1, InpSup1, fim);
   total += DesenhaTF(2, InpTF2, InpVolMA2, InpZones2, InpRes2, InpSup2, fim);
   total += DesenhaTF(3, InpTF3, InpVolMA3, InpZones3, InpRes3, InpSup3, fim);
   total += DesenhaTF(4, InpTF4, InpVolMA4, InpZones4, InpRes4, InpSup4, fim);

   //--- apaga apenas os objetos que NAO foram tocados nesta passada
   //    (zonas que deixaram de existir). Os demais permanecem no lugar,
   //    sem recriacao — e o que elimina o piscar.
   for(int i = ObjectsTotal(0,-1,-1)-1; i >= 0; --i)
   {
      const string nm = ObjectName(0,i);
      if(StringFind(nm, OBJ_PREFIX) != 0) continue;
      if(ObjectGetInteger(0, nm, OBJPROP_ZORDER) != g_gen) ObjectDelete(0, nm);
   }

   static bool avisouVazio = false;
   if(!avisouVazio && total == 0)
   {
      avisouVazio = true;
      Print("[TV_14_VSR] Nenhuma zona encontrada. Causas possiveis: simbolo sem "
            "volume, historico curto nos timeframes escolhidos, ou 'Ocultar TF menor' "
            "escondendo todos (o grafico esta num periodo maior que os configurados).");
   }

   ChartRedraw();
   return rates_total;
}
//+------------------------------------------------------------------+
