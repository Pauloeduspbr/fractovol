# FractoVol

Porte C++/MQL5 de um indicador de price action — zonas de suporte e resistência derivadas de fractais de preço validados por volume — para uso no MetaTrader 5.

**🇧🇷 Português** · [🇺🇸 English](#english)

![FractoVol](screenshot.png)

## O que é

FractoVol é um porte do indicador Pine "Volume-based Support & Resistance Zones V2", publicado na TradingView por tommyf1001 (que credita trabalho anterior de synapticex e Lij_MC como base da lógica original), reimplementado do zero em C++ (motor de cálculo, compilado como DLL x64) e MQL5 (wrapper de plotagem para o MetaTrader 5).

A detecção segue um fractal de Williams de 5 barras:

- Resistência: pivô de topo confirmado por 3 highs sucessivos ascendentes seguidos de 2 highs descendentes.
- Suporte: o mesmo padrão espelhado com lows.
- Confirmação por volume: o fractal só é validado se o volume da barra do pivô superar a média móvel simples de volume, com período configurável.

Cada fractal validado gera uma zona (box) delimitada entre o nível do fractal (o high ou o low do pivô) e a borda do corpo do candle daquela barra (o maior entre open/close para resistência, o menor para suporte) — a distância entre essas duas bordas reflete a significância do nível.

O indicador suporta até 4 timeframes simultâneos (o timeframe atual do gráfico mais 3 configuráveis), com extensão de linhas para a direita, rótulos de timeframe nas zonas ativas e alertas de toque/rompimento configuráveis por timeframe.

No porte, o motor C++ é stateless: processa uma série OHLCV por chamada e devolve as zonas encontradas. O suporte a múltiplos timeframes é resolvido no wrapper MQL5, que chama o motor uma vez por timeframe configurado, alimentando-o com os dados de `CopyRates` daquele período.

## Instalação — versão pré-compilada

1. Copie `volume_sr.dll` para a pasta `MQL5/Libraries` do terminal MetaTrader 5.
2. Copie `TV_14_VolumeSR.ex5` para a pasta `MQL5/Indicators` do mesmo terminal.
3. Reinicie o MetaTrader 5 (ou, no Navegador, atualize/recarregue a lista de indicadores).
4. Arraste o indicador `TV_14_VolumeSR` do Navegador para o gráfico desejado e ajuste os parâmetros de timeframe e volume na janela de propriedades.

## Build a partir do código-fonte

1. Compile o motor C++ (`volume_sr`) com g++/MinGW-w64 usando o `build.sh` incluso em `src/cpp/` — o script gera `volume_sr.dll` (x64).
2. Abra `src/mql5/TV_14_VolumeSR.mq5` no MetaEditor do MetaTrader 5.
3. Compile com F7 para gerar `TV_14_VolumeSR.ex5`.
4. Siga os passos de instalação acima para colocar o `.dll` em `MQL5/Libraries` e o `.ex5` em `MQL5/Indicators`.

## Licença

Este repositório é licenciado sob MIT; a lógica original do indicador, escrita em Pine Script, é de autoria de tommyf1001.

## Aviso

Uso educacional e de análise técnica. Este indicador não constitui recomendação de investimento.

---

## English

C++/MQL5 port of a price action indicator — support and resistance zones derived from volume-validated price fractals — for MetaTrader 5.

### What it is

FractoVol is a port of the Pine indicator "Volume-based Support & Resistance Zones V2", published on TradingView by tommyf1001 (who credits prior work by synapticex and Lij_MC as the basis for the original logic), reimplemented from scratch in C++ (calculation engine, compiled as an x64 DLL) and MQL5 (plotting wrapper for MetaTrader 5).

Detection follows a 5-bar Williams fractal:

- Resistance: a top pivot confirmed by 3 successive rising highs followed by 2 falling highs.
- Support: the same pattern mirrored with lows.
- Volume confirmation: the fractal is only validated if the pivot bar's volume exceeds a simple moving average of volume, with a configurable period.

Every validated fractal generates a zone (box) bounded between the fractal level (the pivot's high or low) and the edge of that bar's candle body (the higher of open/close for resistance, the lower for support) — the distance between these two edges reflects the level's significance.

The indicator supports up to 4 simultaneous timeframes (the chart's current timeframe plus 3 configurable ones), with line extension to the right, timeframe labels on active zones, and configurable touch/breakout alerts per timeframe.

In the port, the C++ engine is stateless: it processes one OHLCV series per call and returns the zones found. Multi-timeframe support is handled in the MQL5 wrapper, which calls the engine once per configured timeframe, feeding it with `CopyRates` data for that period.

### Installation — precompiled version

1. Copy `volume_sr.dll` into the MetaTrader 5 terminal's `MQL5/Libraries` folder.
2. Copy `TV_14_VolumeSR.ex5` into the same terminal's `MQL5/Indicators` folder.
3. Restart MetaTrader 5 (or, in the Navigator, refresh/reload the indicator list).
4. Drag the `TV_14_VolumeSR` indicator from the Navigator onto the chart and adjust the timeframe and volume parameters in the properties window.

### Build from source

1. Compile the C++ engine (`volume_sr`) with g++/MinGW-w64 using the `build.sh` script included in `src/cpp/` — it produces `volume_sr.dll` (x64).
2. Open `src/mql5/TV_14_VolumeSR.mq5` in MetaTrader 5's MetaEditor.
3. Compile with F7 to produce `TV_14_VolumeSR.ex5`.
4. Follow the installation steps above to place the `.dll` in `MQL5/Libraries` and the `.ex5` in `MQL5/Indicators`.

### License

This repository is licensed under MIT; the original indicator logic, written in Pine Script, was authored by tommyf1001.

### Disclaimer

Educational and technical-analysis use only. This indicator does not constitute investment advice.
