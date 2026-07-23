//+------------------------------------------------------------------+
//| volume_sr.h                                                      |
//| Engine C++ do "Volume-based Support & Resistance Zones V2"       |
//| (c) tommyf100, sobre trabalho de synapticex e Lij_MC — Pine v5    |
//|                                                                  |
//| Porte de:                                                        |
//|   indicadores_tradingview/14_Volume_based_Support_Resistance_...  |
//|   source.pine  (922 linhas)                                      |
//|                                                                  |
//| LICENCA: o cabecalho do script nao declara licenca explicita.     |
//| Publicado aberto no TradingView com creditos a synapticex e       |
//| Lij_MC. Tratar como uso pessoal/estudo; NAO redistribuir nem usar |
//| comercialmente sem confirmar com o autor.                        |
//|                                                                  |
//| O QUE O MOTOR FAZ                                                 |
//| Detecta pivos de 5 barras (fractal de Williams) CONFIRMADOS POR   |
//| VOLUME e devolve, para cada um, a zona de suporte/resistencia.    |
//|   resistencia: pivo de topo — high[3]>high[4]>high[5] e           |
//|                high[2]<high[3] e high[1]<high[2]                  |
//|   suporte    : pivo de fundo — espelho do acima com low           |
//|   confirmacao: volume da barra do pivo > SMA(volume, volMaPeriod) |
//| (Pine linhas 258-261.) O pivo fica 3 barras atras da deteccao,    |
//| que e o atraso inerente do fractal: so se sabe que era um topo    |
//| depois de duas barras mais baixas.                                |
//|                                                                   |
//| A ZONA sai do NIVEL ate a BORDA DO CORPO da vela do pivo:         |
//|   resistencia: de high[3] ate max(open[3],close[3])               |
//|   suporte    : de low[3]  ate min(open[3],close[3])               |
//| (Pine 390-405: `Close[3] >= Open[3] ? Close[3] : Open[3]` para a  |
//| resistencia, e o inverso para o suporte.)                        |
//|                                                                   |
//| MULTI-TIMEFRAME: o Pine usa request.security para ler 4 periodos. |
//| Aqui a DLL processa UMA serie por chamada; o wrapper MQL5 chama   |
//| uma vez por timeframe, com os dados vindos de CopyRates daquele   |
//| periodo. Fica mais simples e sem nenhum estado.                   |
//|                                                                   |
//| SEM ESTADO entre chamadas. Fronteira: int/double/ponteiros.       |
//+------------------------------------------------------------------+
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define VSR_ABI_VERSION 1
#define VSR_RESISTANCE  1
#define VSR_SUPPORT    -1

__declspec(dllexport) int __stdcall VsrVersion(void);

//+------------------------------------------------------------------+
//| VsrScan — varre a serie e devolve as zonas encontradas            |
//|                                                                   |
//| ENTRADA (indice 0 = barra mais ANTIGA)                            |
//|   open/high/low/close/volume, size                                |
//|   volMaPeriod  janela da media de volume (Pine: defval 6)         |
//|                                                                   |
//| SAIDA — arrays paralelos, uma posicao por zona, em ordem          |
//| CRONOLOGICA (a mais antiga primeiro):                             |
//|   outPivotBar  indice da barra do pivo (onde a zona comeca)       |
//|   outFoundBar  indice em que o fractal foi CONFIRMADO (pivo+3)    |
//|   outType      VSR_RESISTANCE (+1) ou VSR_SUPPORT (-1)            |
//|   outLevel     high do pivo (resistencia) ou low (suporte)        |
//|   outZone      borda do corpo da vela do pivo                     |
//|   Qualquer ponteiro pode ser NULL.                                |
//|                                                                   |
//| capacity limita quantas zonas cabem na saida. Se houver mais      |
//| pivos que isso, as MAIS ANTIGAS sao descartadas (o Pine tambem    |
//| descarta, via array.shift com TF_NumZones).                       |
//|                                                                   |
//| Retorno: numero de zonas escritas; -1 em argumento invalido.      |
//+------------------------------------------------------------------+
__declspec(dllexport) int __stdcall VsrScan(
   const double *open, const double *high, const double *low,
   const double *close, const double *volume, int size,
   int volMaPeriod,
   double *outPivotBar, double *outFoundBar, double *outType,
   double *outLevel, double *outZone, int capacity);

#ifdef __cplusplus
}
#endif
