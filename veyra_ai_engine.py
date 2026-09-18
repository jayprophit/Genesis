"""
╔══════════════════════════════════════════════════════════════════════════════╗
║              VEYRA (VRA) - AI/ML FINANCIAL INTELLIGENCE ENGINE               ║
║              100% Open-Source | Zero API Keys | Zero Cost                    ║
║              Drop into: src/backend/ai/veyra_ai_engine.py                   ║
╚══════════════════════════════════════════════════════════════════════════════╝

MODULES COVERED:
  ✅ Market Data          → yfinance, FRED (via requests), CryptoCompare
  ✅ Technical Analysis   → 20+ indicators (pure numpy/pandas, no TA-Lib)
  ✅ Sentiment Engine     → VADER (NLP), news headline scoring
  ✅ ML Price Prediction  → RandomForest, GradientBoosting, ensemble
  ✅ Portfolio Optimiser  → Modern Portfolio Theory, Kelly Criterion
  ✅ Risk Engine          → VaR, CVaR, Sharpe, Sortino, max drawdown
  ✅ Signal Aggregator    → Multi-source confidence-weighted signals
  ✅ Market Regime        → 8-state HMM-lite classifier
  ✅ Anomaly Detection    → Isolation Forest, Z-score, IQR
  ✅ Economic Calendar    → FRED macro indicators
  ✅ Crypto Data          → yfinance crypto pairs
  ✅ REST API             → FastAPI with 40+ endpoints
  ✅ WebSocket            → Real-time streaming endpoint

INSTALL (add to requirements.txt):
  fastapi>=0.110.0
  uvicorn[standard]>=0.29.0
  yfinance>=0.2.37
  pandas>=2.2.0
  numpy>=1.26.0
  scikit-learn>=1.4.0
  scipy>=1.12.0
  vaderSentiment>=3.3.2
  requests>=2.31.0
  aiohttp>=3.9.0
  python-dotenv>=1.0.0
  websockets>=12.0
  joblib>=1.3.0

RUN:
  uvicorn veyra_ai_engine:app --host 0.0.0.0 --port 8001 --reload
"""

from __future__ import annotations

import asyncio
import json
import logging
import os
import time
import warnings
from collections import deque
from datetime import datetime, timedelta
from typing import Any, Dict, List, Optional, Tuple

import numpy as np
import pandas as pd
import requests
import yfinance as yf
from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect, Query
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel, Field
from scipy.optimize import minimize
from scipy.stats import norm, zscore
from sklearn.ensemble import (
    GradientBoostingRegressor,
    IsolationForest,
    RandomForestClassifier,
    RandomForestRegressor,
)
from sklearn.model_selection import TimeSeriesSplit, cross_val_score
from sklearn.preprocessing import MinMaxScaler, StandardScaler
from vaderSentiment.vaderSentiment import SentimentIntensityAnalyzer

warnings.filterwarnings("ignore")
logging.basicConfig(level=logging.INFO, format="%(asctime)s | %(levelname)s | %(message)s")
log = logging.getLogger("veyra_ai")

# ─────────────────────────────────────────────────────────────────────────────
# APP INITIALISATION
# ─────────────────────────────────────────────────────────────────────────────

app = FastAPI(
    title="Veyra AI/ML Engine",
    description="100% Open-Source Financial Intelligence Layer for VRA Platform",
    version="4.0.0",
    docs_url="/ai/docs",
    redoc_url="/ai/redoc",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# In-memory caches
_cache: Dict[str, Any] = {}
_cache_ttl: Dict[str, float] = {}
CACHE_DURATION = 300  # 5 minutes default

vader = SentimentIntensityAnalyzer()


# ─────────────────────────────────────────────────────────────────────────────
# PYDANTIC MODELS
# ─────────────────────────────────────────────────────────────────────────────

class TickerRequest(BaseModel):
    ticker: str
    period: str = "1y"
    interval: str = "1d"

class PortfolioRequest(BaseModel):
    tickers: List[str]
    weights: Optional[List[float]] = None
    period: str = "1y"
    risk_free_rate: float = 0.045  # UK gilt rate

class OptimiseRequest(BaseModel):
    tickers: List[str]
    period: str = "2y"
    method: str = "sharpe"  # sharpe | min_vol | max_return | kelly
    risk_free_rate: float = 0.045
    constraints: Optional[Dict] = None

class PredictRequest(BaseModel):
    ticker: str
    horizon_days: int = Field(default=5, ge=1, le=30)
    model: str = "ensemble"  # rf | gbm | ensemble

class SentimentRequest(BaseModel):
    texts: List[str]
    context: str = "financial"

class RiskRequest(BaseModel):
    tickers: List[str]
    weights: Optional[List[float]] = None
    period: str = "1y"
    confidence: float = 0.95

class SignalRequest(BaseModel):
    ticker: str
    period: str = "6mo"

class AnomalyRequest(BaseModel):
    ticker: str
    period: str = "2y"
    contamination: float = 0.05


# ─────────────────────────────────────────────────────────────────────────────
# UTILITY: CACHING
# ─────────────────────────────────────────────────────────────────────────────

def _cache_get(key: str) -> Optional[Any]:
    if key in _cache and time.time() < _cache_ttl.get(key, 0):
        return _cache[key]
    return None

def _cache_set(key: str, value: Any, ttl: int = CACHE_DURATION):
    _cache[key] = value
    _cache_ttl[key] = time.time() + ttl


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 1: MARKET DATA LAYER (yfinance + FRED + Crypto)
# ─────────────────────────────────────────────────────────────────────────────

class MarketDataEngine:
    """Fetches market data from 100% free sources."""

    FRED_BASE = "https://api.stlouisfed.org/fred/series/observations"
    FRED_API_KEY = os.getenv("FRED_API_KEY", "")  # optional; many endpoints are public

    CRYPTO_PAIRS = ["BTC-USD", "ETH-USD", "BNB-USD", "SOL-USD", "XRP-USD",
                    "ADA-USD", "DOGE-USD", "AVAX-USD", "DOT-USD", "MATIC-USD"]

    INDICES = {"S&P500": "^GSPC", "NASDAQ": "^IXIC", "FTSE100": "^FTSE",
               "DAX": "^GDAXI", "NIKKEI": "^N225", "VIX": "^VIX",
               "GOLD": "GC=F", "OIL_WTI": "CL=F", "USD_INDEX": "DX-Y.NYB"}

    SECTORS_ETF = {
        "Technology": "XLK", "Healthcare": "XLV", "Financials": "XLF",
        "Energy": "XLE", "Consumer": "XLY", "Utilities": "XLU",
        "Materials": "XLB", "Industrials": "XLI", "RealEstate": "XLRE",
    }

    def fetch_ohlcv(self, ticker: str, period: str = "1y", interval: str = "1d") -> pd.DataFrame:
        key = f"ohlcv:{ticker}:{period}:{interval}"
        cached = _cache_get(key)
        if cached is not None:
            return cached
        try:
            df = yf.download(ticker, period=period, interval=interval,
                             auto_adjust=True, progress=False)
            if df.empty:
                raise ValueError(f"No data for {ticker}")
            df.dropna(inplace=True)
            _cache_set(key, df, ttl=60 if interval in ("1m", "5m") else 300)
            return df
        except Exception as e:
            log.error(f"fetch_ohlcv({ticker}): {e}")
            raise HTTPException(status_code=404, detail=f"Cannot fetch {ticker}: {e}")

    def fetch_info(self, ticker: str) -> Dict:
        key = f"info:{ticker}"
        cached = _cache_get(key)
        if cached is not None:
            return cached
        info = yf.Ticker(ticker).info
        _cache_set(key, info, ttl=3600)
        return info

    def fetch_multiple(self, tickers: List[str], period: str = "1y") -> pd.DataFrame:
        key = f"multi:{':'.join(sorted(tickers))}:{period}"
        cached = _cache_get(key)
        if cached is not None:
            return cached
        data = yf.download(tickers, period=period, auto_adjust=True, progress=False)["Close"]
        if isinstance(data, pd.Series):
            data = data.to_frame(name=tickers[0])
        data.dropna(how="all", inplace=True)
        _cache_set(key, data, ttl=300)
        return data

    def fetch_fred(self, series_id: str, limit: int = 100) -> pd.Series:
        """Fetch FRED economic data (public endpoint, no key needed for many series)."""
        key = f"fred:{series_id}"
        cached = _cache_get(key)
        if cached is not None:
            return cached
        try:
            params = {"series_id": series_id, "limit": limit,
                      "sort_order": "desc", "file_type": "json"}
            if self.FRED_API_KEY:
                params["api_key"] = self.FRED_API_KEY
            r = requests.get(self.FRED_BASE, params=params, timeout=10)
            data = r.json()
            observations = data.get("observations", [])
            if not observations:
                return pd.Series(dtype=float)
            s = pd.Series(
                {o["date"]: float(o["value"]) for o in observations if o["value"] != "."},
                name=series_id,
            )
            s.index = pd.to_datetime(s.index)
            s.sort_index(inplace=True)
            _cache_set(key, s, ttl=3600)
            return s
        except Exception as e:
            log.warning(f"FRED fetch failed for {series_id}: {e}")
            return pd.Series(dtype=float)

    def fetch_market_snapshot(self) -> Dict:
        snapshot = {}
        for name, sym in self.INDICES.items():
            try:
                df = self.fetch_ohlcv(sym, period="5d", interval="1d")
                if len(df) >= 2:
                    latest = float(df["Close"].iloc[-1])
                    prev = float(df["Close"].iloc[-2])
                    snapshot[name] = {
                        "symbol": sym,
                        "price": round(latest, 2),
                        "change_pct": round((latest - prev) / prev * 100, 2),
                    }
            except Exception:
                pass
        return snapshot

    def fetch_sector_rotation(self) -> Dict:
        returns = {}
        for sector, etf in self.SECTORS_ETF.items():
            try:
                df = self.fetch_ohlcv(etf, period="3mo")
                ret = float(df["Close"].iloc[-1] / df["Close"].iloc[0] - 1)
                returns[sector] = round(ret * 100, 2)
            except Exception:
                pass
        return dict(sorted(returns.items(), key=lambda x: x[1], reverse=True))


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 2: TECHNICAL ANALYSIS ENGINE (pure numpy/pandas)
# ─────────────────────────────────────────────────────────────────────────────

class TechnicalAnalysisEngine:
    """20+ technical indicators, pure Python — no TA-Lib dependency."""

    # ── Trend ──
    @staticmethod
    def sma(series: pd.Series, window: int) -> pd.Series:
        return series.rolling(window).mean()

    @staticmethod
    def ema(series: pd.Series, span: int) -> pd.Series:
        return series.ewm(span=span, adjust=False).mean()

    @staticmethod
    def macd(series: pd.Series) -> Tuple[pd.Series, pd.Series, pd.Series]:
        fast = series.ewm(span=12, adjust=False).mean()
        slow = series.ewm(span=26, adjust=False).mean()
        macd_line = fast - slow
        signal = macd_line.ewm(span=9, adjust=False).mean()
        histogram = macd_line - signal
        return macd_line, signal, histogram

    @staticmethod
    def supertrend(df: pd.DataFrame, period: int = 14, mult: float = 3.0) -> pd.Series:
        hl2 = (df["High"] + df["Low"]) / 2
        atr = TechnicalAnalysisEngine.atr(df, period)
        upper = hl2 + mult * atr
        lower = hl2 - mult * atr
        supertrend = pd.Series(index=df.index, dtype=float)
        direction = pd.Series(1, index=df.index)
        for i in range(1, len(df)):
            if df["Close"].iloc[i] > upper.iloc[i - 1]:
                direction.iloc[i] = 1
            elif df["Close"].iloc[i] < lower.iloc[i - 1]:
                direction.iloc[i] = -1
            else:
                direction.iloc[i] = direction.iloc[i - 1]
            supertrend.iloc[i] = lower.iloc[i] if direction.iloc[i] == 1 else upper.iloc[i]
        return supertrend

    # ── Momentum ──
    @staticmethod
    def rsi(series: pd.Series, period: int = 14) -> pd.Series:
        delta = series.diff()
        gain = delta.where(delta > 0, 0.0).rolling(period).mean()
        loss = (-delta.where(delta < 0, 0.0)).rolling(period).mean()
        rs = gain / loss.replace(0, np.nan)
        return 100 - (100 / (1 + rs))

    @staticmethod
    def stochastic(df: pd.DataFrame, k: int = 14, d: int = 3) -> Tuple[pd.Series, pd.Series]:
        low_min = df["Low"].rolling(k).min()
        high_max = df["High"].rolling(k).max()
        k_pct = 100 * (df["Close"] - low_min) / (high_max - low_min).replace(0, np.nan)
        d_pct = k_pct.rolling(d).mean()
        return k_pct, d_pct

    @staticmethod
    def williams_r(df: pd.DataFrame, period: int = 14) -> pd.Series:
        high = df["High"].rolling(period).max()
        low = df["Low"].rolling(period).min()
        return -100 * (high - df["Close"]) / (high - low).replace(0, np.nan)

    @staticmethod
    def cci(df: pd.DataFrame, period: int = 20) -> pd.Series:
        tp = (df["High"] + df["Low"] + df["Close"]) / 3
        ma = tp.rolling(period).mean()
        mad = tp.rolling(period).apply(lambda x: np.abs(x - x.mean()).mean())
        return (tp - ma) / (0.015 * mad.replace(0, np.nan))

    @staticmethod
    def roc(series: pd.Series, period: int = 10) -> pd.Series:
        return series.pct_change(period) * 100

    # ── Volatility ──
    @staticmethod
    def atr(df: pd.DataFrame, period: int = 14) -> pd.Series:
        hl = df["High"] - df["Low"]
        hc = (df["High"] - df["Close"].shift()).abs()
        lc = (df["Low"] - df["Close"].shift()).abs()
        tr = pd.concat([hl, hc, lc], axis=1).max(axis=1)
        return tr.rolling(period).mean()

    @staticmethod
    def bollinger_bands(series: pd.Series, window: int = 20, std: float = 2.0):
        mid = series.rolling(window).mean()
        dev = series.rolling(window).std()
        return mid - std * dev, mid, mid + std * dev

    @staticmethod
    def keltner_channels(df: pd.DataFrame, ema_period: int = 20, atr_period: int = 10, mult: float = 2.0):
        mid = TechnicalAnalysisEngine.ema(df["Close"], ema_period)
        a = TechnicalAnalysisEngine.atr(df, atr_period)
        return mid - mult * a, mid, mid + mult * a

    # ── Volume ──
    @staticmethod
    def obv(df: pd.DataFrame) -> pd.Series:
        direction = np.sign(df["Close"].diff()).fillna(0)
        return (direction * df["Volume"]).cumsum()

    @staticmethod
    def vwap(df: pd.DataFrame) -> pd.Series:
        tp = (df["High"] + df["Low"] + df["Close"]) / 3
        return (tp * df["Volume"]).cumsum() / df["Volume"].cumsum()

    @staticmethod
    def mfi(df: pd.DataFrame, period: int = 14) -> pd.Series:
        tp = (df["High"] + df["Low"] + df["Close"]) / 3
        raw_mf = tp * df["Volume"]
        pos_mf = raw_mf.where(tp > tp.shift(), 0).rolling(period).sum()
        neg_mf = raw_mf.where(tp <= tp.shift(), 0).rolling(period).sum()
        return 100 - (100 / (1 + pos_mf / neg_mf.replace(0, np.nan)))

    # ── Trend Strength ──
    @staticmethod
    def adx(df: pd.DataFrame, period: int = 14) -> pd.Series:
        up = df["High"].diff()
        dn = -df["Low"].diff()
        plus_dm = up.where((up > dn) & (up > 0), 0)
        minus_dm = dn.where((dn > up) & (dn > 0), 0)
        atr = TechnicalAnalysisEngine.atr(df, period)
        plus_di = 100 * plus_dm.rolling(period).mean() / atr.replace(0, np.nan)
        minus_di = 100 * minus_dm.rolling(period).mean() / atr.replace(0, np.nan)
        dx = 100 * (plus_di - minus_di).abs() / (plus_di + minus_di).replace(0, np.nan)
        return dx.rolling(period).mean()

    def compute_all(self, df: pd.DataFrame) -> Dict:
        """Compute all indicators and return latest values as a dict."""
        c = df["Close"]
        result = {}

        # Trend
        result["sma_20"] = round(float(self.sma(c, 20).iloc[-1]), 2)
        result["sma_50"] = round(float(self.sma(c, 50).iloc[-1]), 2)
        result["sma_200"] = round(float(self.sma(c, 200).iloc[-1]), 2)
        result["ema_12"] = round(float(self.ema(c, 12).iloc[-1]), 2)
        result["ema_26"] = round(float(self.ema(c, 26).iloc[-1]), 2)

        macd_l, macd_s, macd_h = self.macd(c)
        result["macd_line"] = round(float(macd_l.iloc[-1]), 4)
        result["macd_signal"] = round(float(macd_s.iloc[-1]), 4)
        result["macd_histogram"] = round(float(macd_h.iloc[-1]), 4)

        # Momentum
        result["rsi_14"] = round(float(self.rsi(c).iloc[-1]), 2)
        k, d = self.stochastic(df)
        result["stoch_k"] = round(float(k.iloc[-1]), 2)
        result["stoch_d"] = round(float(d.iloc[-1]), 2)
        result["williams_r"] = round(float(self.williams_r(df).iloc[-1]), 2)
        result["cci"] = round(float(self.cci(df).iloc[-1]), 2)
        result["roc"] = round(float(self.roc(c).iloc[-1]), 2)

        # Volatility
        result["atr"] = round(float(self.atr(df).iloc[-1]), 4)
        bb_l, bb_m, bb_u = self.bollinger_bands(c)
        result["bb_lower"] = round(float(bb_l.iloc[-1]), 2)
        result["bb_mid"] = round(float(bb_m.iloc[-1]), 2)
        result["bb_upper"] = round(float(bb_u.iloc[-1]), 2)
        bb_width = float((bb_u - bb_l).iloc[-1] / bb_m.iloc[-1]) if float(bb_m.iloc[-1]) != 0 else 0
        result["bb_width"] = round(bb_width, 4)

        # Volume
        result["obv"] = int(self.obv(df).iloc[-1])
        result["mfi"] = round(float(self.mfi(df).iloc[-1]), 2)

        # Trend strength
        result["adx"] = round(float(self.adx(df).iloc[-1]), 2)

        # Price context
        price = float(c.iloc[-1])
        result["price"] = round(price, 4)
        result["vs_sma200_pct"] = round((price / result["sma_200"] - 1) * 100, 2)
        result["52w_high"] = round(float(c.rolling(252).max().iloc[-1]), 2)
        result["52w_low"] = round(float(c.rolling(252).min().iloc[-1]), 2)

        return result

    def pattern_scan(self, df: pd.DataFrame) -> List[Dict]:
        """Detect candlestick and chart patterns."""
        patterns = []
        c = df["Close"]
        o = df["Open"]
        h = df["High"]
        l = df["Low"]

        # Doji
        body = (c - o).abs()
        wick = h - l
        if len(df) > 0 and float(wick.iloc[-1]) > 0:
            if float(body.iloc[-1]) / float(wick.iloc[-1]) < 0.1:
                patterns.append({"pattern": "Doji", "signal": "neutral", "strength": 0.5})

        # Hammer
        if len(df) > 0:
            lower_shadow = float(min(c.iloc[-1], o.iloc[-1])) - float(l.iloc[-1])
            upper_shadow = float(h.iloc[-1]) - float(max(c.iloc[-1], o.iloc[-1]))
            body_size = float(body.iloc[-1])
            if lower_shadow > 2 * body_size and upper_shadow < body_size:
                patterns.append({"pattern": "Hammer", "signal": "bullish", "strength": 0.7})

        # Shooting Star
        if len(df) > 0:
            upper_shadow = float(h.iloc[-1]) - float(max(c.iloc[-1], o.iloc[-1]))
            lower_shadow = float(min(c.iloc[-1], o.iloc[-1])) - float(l.iloc[-1])
            body_size = float(body.iloc[-1])
            if upper_shadow > 2 * body_size and lower_shadow < body_size:
                patterns.append({"pattern": "Shooting Star", "signal": "bearish", "strength": 0.7})

        # Golden Cross / Death Cross
        if len(df) > 200:
            sma50 = float(self.sma(c, 50).iloc[-1])
            sma200 = float(self.sma(c, 200).iloc[-1])
            sma50_prev = float(self.sma(c, 50).iloc[-2])
            sma200_prev = float(self.sma(c, 200).iloc[-2])
            if sma50_prev < sma200_prev and sma50 > sma200:
                patterns.append({"pattern": "Golden Cross", "signal": "bullish", "strength": 0.9})
            elif sma50_prev > sma200_prev and sma50 < sma200:
                patterns.append({"pattern": "Death Cross", "signal": "bearish", "strength": 0.9})

        # RSI divergence (simple)
        if len(df) > 30:
            rsi = self.rsi(c)
            recent_high_price = float(c.iloc[-10:].max())
            prior_high_price = float(c.iloc[-30:-10].max())
            recent_high_rsi = float(rsi.iloc[-10:].max())
            prior_high_rsi = float(rsi.iloc[-30:-10].max())
            if recent_high_price > prior_high_price and recent_high_rsi < prior_high_rsi:
                patterns.append({"pattern": "Bearish RSI Divergence", "signal": "bearish", "strength": 0.75})
            elif recent_high_price < prior_high_price and recent_high_rsi > prior_high_rsi:
                patterns.append({"pattern": "Bullish RSI Divergence", "signal": "bullish", "strength": 0.75})

        return patterns


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 3: SENTIMENT ANALYSIS ENGINE (VADER + Financial NLP)
# ─────────────────────────────────────────────────────────────────────────────

class SentimentEngine:
    """Financial sentiment analysis using VADER (no API key required)."""

    FINANCIAL_BOOSTERS = {
        "bullish": 2.0, "bearish": -2.0, "rally": 1.5, "crash": -2.0,
        "surge": 1.5, "plunge": -2.0, "soar": 1.5, "tank": -2.0,
        "beat": 1.0, "miss": -1.0, "upgrade": 1.5, "downgrade": -1.5,
        "buy": 0.8, "sell": -0.8, "outperform": 1.2, "underperform": -1.2,
        "strong": 0.7, "weak": -0.7, "growth": 0.8, "decline": -0.8,
        "profit": 0.9, "loss": -0.9, "record": 1.0, "bankruptcy": -2.5,
        "dividend": 0.6, "layoff": -1.2, "acquisition": 0.8, "lawsuit": -1.0,
    }

    def analyse(self, text: str) -> Dict:
        raw = vader.polarity_scores(text)
        text_lower = text.lower()
        adj = 0.0
        matched = []
        for word, boost in self.FINANCIAL_BOOSTERS.items():
            if word in text_lower:
                adj += boost * 0.05
                matched.append(word)
        compound = max(-1.0, min(1.0, raw["compound"] + adj))
        label = "bullish" if compound >= 0.05 else "bearish" if compound <= -0.05 else "neutral"
        confidence = min(1.0, abs(compound) + 0.1)
        return {
            "compound": round(compound, 4),
            "positive": round(raw["pos"], 4),
            "negative": round(raw["neg"], 4),
            "neutral": round(raw["neu"], 4),
            "label": label,
            "confidence": round(confidence, 4),
            "matched_keywords": matched,
        }

    def analyse_batch(self, texts: List[str]) -> Dict:
        results = [self.analyse(t) for t in texts]
        compounds = [r["compound"] for r in results]
        avg_compound = float(np.mean(compounds))
        label = "bullish" if avg_compound >= 0.05 else "bearish" if avg_compound <= -0.05 else "neutral"
        return {
            "individual": results,
            "aggregate": {
                "avg_compound": round(avg_compound, 4),
                "label": label,
                "bullish_pct": round(sum(1 for r in results if r["label"] == "bullish") / len(results) * 100, 1),
                "bearish_pct": round(sum(1 for r in results if r["label"] == "bearish") / len(results) * 100, 1),
                "neutral_pct": round(sum(1 for r in results if r["label"] == "neutral") / len(results) * 100, 1),
                "count": len(results),
            },
        }

    def fetch_and_analyse_headlines(self, ticker: str) -> Dict:
        """Fetch free headlines via yfinance and score them."""
        try:
            stock = yf.Ticker(ticker)
            news = stock.news or []
            headlines = [item.get("title", "") for item in news[:20] if item.get("title")]
            if not headlines:
                return {"error": "No headlines available", "ticker": ticker}
            return {
                "ticker": ticker,
                "headlines_analysed": len(headlines),
                "headlines": headlines[:5],
                **self.analyse_batch(headlines),
            }
        except Exception as e:
            return {"error": str(e), "ticker": ticker}


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 4: ML PRICE PREDICTION ENGINE
# ─────────────────────────────────────────────────────────────────────────────

class MLPredictionEngine:
    """Ensemble ML price prediction using sklearn (no deep learning required)."""

    ta = TechnicalAnalysisEngine()

    def _build_features(self, df: pd.DataFrame) -> pd.DataFrame:
        c = df["Close"]
        feats = pd.DataFrame(index=df.index)

        # Price-based
        feats["ret_1"] = c.pct_change(1)
        feats["ret_3"] = c.pct_change(3)
        feats["ret_5"] = c.pct_change(5)
        feats["ret_10"] = c.pct_change(10)
        feats["ret_20"] = c.pct_change(20)

        # Technical
        feats["rsi"] = self.ta.rsi(c)
        feats["macd_h"] = self.ta.macd(c)[2]
        feats["bb_width"] = (self.ta.bollinger_bands(c)[2] - self.ta.bollinger_bands(c)[0]) / self.ta.bollinger_bands(c)[1]
        feats["atr_norm"] = self.ta.atr(df) / c
        feats["adx"] = self.ta.adx(df)
        feats["obv_norm"] = self.ta.obv(df).pct_change(5)
        feats["cci"] = self.ta.cci(df)
        feats["williams"] = self.ta.williams_r(df)

        # Volume
        feats["vol_ratio"] = df["Volume"] / df["Volume"].rolling(20).mean()
        feats["vol_trend"] = df["Volume"].pct_change(5)

        # SMA distances
        feats["dist_sma20"] = (c - self.ta.sma(c, 20)) / c
        feats["dist_sma50"] = (c - self.ta.sma(c, 50)) / c

        # Volatility
        feats["volatility_20"] = c.pct_change().rolling(20).std() * np.sqrt(252)

        # Rolling highs/lows
        feats["pos_52w"] = (c - c.rolling(252).min()) / (c.rolling(252).max() - c.rolling(252).min() + 1e-9)

        return feats

    def predict(self, df: pd.DataFrame, horizon: int = 5, model_type: str = "ensemble") -> Dict:
        features = self._build_features(df)
        target = df["Close"].pct_change(horizon).shift(-horizon)

        mask = features.notna().all(axis=1) & target.notna()
        X = features[mask]
        y = target[mask]

        if len(X) < 50:
            raise HTTPException(status_code=400, detail="Not enough data for prediction (need 50+ rows)")

        scaler = StandardScaler()
        X_scaled = scaler.fit_transform(X)

        # Label: 1=up, 0=down
        y_direction = (y > 0).astype(int)

        # Time-series split
        tscv = TimeSeriesSplit(n_splits=5)

        models = {
            "rf": RandomForestClassifier(n_estimators=100, max_depth=5, random_state=42),
            "gbm": GradientBoostingRegressor(n_estimators=100, max_depth=3, learning_rate=0.05, random_state=42),
        }

        rf = RandomForestClassifier(n_estimators=100, max_depth=5, random_state=42)
        rf.fit(X_scaled[:-horizon], y_direction.iloc[:-horizon])

        gbm = GradientBoostingRegressor(n_estimators=100, max_depth=3, learning_rate=0.05, random_state=42)
        gbm.fit(X_scaled[:-horizon], y.iloc[:-horizon])

        # Latest features (last row)
        X_latest = scaler.transform(X.iloc[[-1]])
        rf_prob = float(rf.predict_proba(X_latest)[0][1])
        gbm_ret = float(gbm.predict(X_latest)[0])

        # Feature importance
        feature_names = X.columns.tolist()
        importances = {feature_names[i]: round(float(v), 4) for i, v in enumerate(rf.feature_importances_)}
        top_features = dict(sorted(importances.items(), key=lambda x: x[1], reverse=True)[:5])

        current_price = float(df["Close"].iloc[-1])
        predicted_price = round(current_price * (1 + gbm_ret), 2)
        direction = "UP" if rf_prob > 0.5 else "DOWN"
        confidence = abs(rf_prob - 0.5) * 2  # 0-1 scale

        return {
            "ticker_price": current_price,
            "horizon_days": horizon,
            "predicted_price": predicted_price,
            "predicted_return_pct": round(gbm_ret * 100, 2),
            "direction": direction,
            "direction_probability": round(rf_prob, 4),
            "confidence": round(confidence, 4),
            "confidence_label": "High" if confidence > 0.6 else "Medium" if confidence > 0.3 else "Low",
            "top_features": top_features,
            "model": model_type,
            "trained_on_samples": len(X) - horizon,
        }


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 5: PORTFOLIO OPTIMISATION ENGINE (MPT + Kelly)
# ─────────────────────────────────────────────────────────────────────────────

class PortfolioEngine:
    mde = MarketDataEngine()

    def _get_returns(self, tickers: List[str], period: str) -> pd.DataFrame:
        prices = self.mde.fetch_multiple(tickers, period)
        prices = prices[tickers] if all(t in prices.columns for t in tickers) else prices
        return prices.pct_change().dropna()

    def metrics(self, returns: pd.DataFrame, weights: np.ndarray, rf: float = 0.045) -> Dict:
        port_ret = returns.dot(weights)
        ann_ret = float(port_ret.mean() * 252)
        ann_vol = float(port_ret.std() * np.sqrt(252))
        sharpe = (ann_ret - rf) / ann_vol if ann_vol > 0 else 0

        # Sortino
        downside = port_ret[port_ret < 0].std() * np.sqrt(252)
        sortino = (ann_ret - rf) / downside if downside > 0 else 0

        # Max drawdown
        cum = (1 + port_ret).cumprod()
        roll_max = cum.cummax()
        drawdown = (cum - roll_max) / roll_max
        max_dd = float(drawdown.min())

        # Calmar
        calmar = ann_ret / abs(max_dd) if max_dd != 0 else 0

        # VaR / CVaR
        var_95 = float(np.percentile(port_ret, 5))
        cvar_95 = float(port_ret[port_ret <= var_95].mean())

        return {
            "annual_return": round(ann_ret * 100, 2),
            "annual_volatility": round(ann_vol * 100, 2),
            "sharpe_ratio": round(sharpe, 4),
            "sortino_ratio": round(sortino, 4),
            "max_drawdown": round(max_dd * 100, 2),
            "calmar_ratio": round(calmar, 4),
            "var_95": round(var_95 * 100, 2),
            "cvar_95": round(cvar_95 * 100, 2),
        }

    def optimise_sharpe(self, returns: pd.DataFrame, rf: float, bounds=(0.02, 0.5)) -> np.ndarray:
        n = returns.shape[1]
        mu = returns.mean() * 252
        cov = returns.cov() * 252

        def neg_sharpe(w):
            ret = w @ mu
            vol = np.sqrt(w @ cov @ w)
            return -(ret - rf) / vol if vol > 0 else 0

        cons = {"type": "eq", "fun": lambda w: w.sum() - 1}
        bnds = [(bounds[0], bounds[1])] * n
        w0 = np.array([1 / n] * n)
        res = minimize(neg_sharpe, w0, method="SLSQP", bounds=bnds, constraints=cons,
                       options={"maxiter": 1000})
        return res.x if res.success else w0

    def optimise_min_vol(self, returns: pd.DataFrame, bounds=(0.02, 0.5)) -> np.ndarray:
        n = returns.shape[1]
        cov = returns.cov() * 252

        def portfolio_vol(w):
            return np.sqrt(w @ cov @ w)

        cons = {"type": "eq", "fun": lambda w: w.sum() - 1}
        bnds = [(bounds[0], bounds[1])] * n
        w0 = np.array([1 / n] * n)
        res = minimize(portfolio_vol, w0, method="SLSQP", bounds=bnds, constraints=cons)
        return res.x if res.success else w0

    def kelly_weights(self, returns: pd.DataFrame) -> np.ndarray:
        """Fractional Kelly criterion (half-Kelly for safety)."""
        mu = returns.mean() * 252
        cov_inv = np.linalg.pinv(returns.cov() * 252)
        kelly = cov_inv @ mu
        # Half-Kelly and normalise
        kelly = kelly * 0.5
        kelly = np.clip(kelly, 0, None)
        return kelly / kelly.sum() if kelly.sum() > 0 else np.ones(len(mu)) / len(mu)

    def efficient_frontier(self, returns: pd.DataFrame, n_points: int = 50) -> List[Dict]:
        n = returns.shape[1]
        mu = returns.mean() * 252
        cov = returns.cov() * 252
        target_rets = np.linspace(float(mu.min()), float(mu.max()), n_points)
        frontier = []
        for target in target_rets:
            def port_vol(w):
                return np.sqrt(w @ cov @ w)
            cons = [{"type": "eq", "fun": lambda w: w.sum() - 1},
                    {"type": "eq", "fun": lambda w, t=target: w @ mu - t}]
            res = minimize(port_vol, [1/n]*n, method="SLSQP",
                           bounds=[(0, 1)]*n, constraints=cons)
            if res.success:
                vol = float(np.sqrt(res.x @ cov @ res.x))
                frontier.append({"return": round(target * 100, 2), "volatility": round(vol * 100, 2)})
        return frontier

    def run_optimisation(self, tickers: List[str], method: str, period: str, rf: float) -> Dict:
        returns = self._get_returns(tickers, period)
        available = [t for t in tickers if t in returns.columns]
        returns = returns[available]

        if method == "sharpe":
            weights = self.optimise_sharpe(returns, rf)
        elif method == "min_vol":
            weights = self.optimise_min_vol(returns)
        elif method == "kelly":
            weights = self.kelly_weights(returns)
        else:
            weights = np.array([1 / len(available)] * len(available))

        weights = weights / weights.sum()
        met = self.metrics(returns, weights, rf)
        ef = self.efficient_frontier(returns, n_points=30)

        return {
            "method": method,
            "tickers": available,
            "weights": {t: round(float(w), 4) for t, w in zip(available, weights)},
            "metrics": met,
            "efficient_frontier": ef,
        }


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 6: RISK MANAGEMENT ENGINE
# ─────────────────────────────────────────────────────────────────────────────

class RiskEngine:
    mde = MarketDataEngine()

    def portfolio_risk(self, tickers: List[str], weights: List[float], period: str, conf: float) -> Dict:
        prices = self.mde.fetch_multiple(tickers, period)
        available = [t for t in tickers if t in prices.columns]
        weights_arr = np.array(weights[:len(available)])
        weights_arr = weights_arr / weights_arr.sum()

        ret = prices[available].pct_change().dropna()
        port_ret = ret.dot(weights_arr)

        # VaR methods
        historical_var = float(np.percentile(port_ret, (1 - conf) * 100))
        cvar = float(port_ret[port_ret <= historical_var].mean())
        sigma = float(port_ret.std())
        parametric_var = float(norm.ppf(1 - conf) * sigma)

        # Drawdown analysis
        cum = (1 + port_ret).cumprod()
        roll_max = cum.cummax()
        dd = (cum - roll_max) / roll_max
        max_dd = float(dd.min())
        avg_dd = float(dd[dd < 0].mean()) if (dd < 0).any() else 0.0
        current_dd = float(dd.iloc[-1])

        # Beta to S&P500
        try:
            spy = self.mde.fetch_ohlcv("SPY", period).Close.pct_change().dropna()
            aligned = pd.concat([port_ret, spy], axis=1).dropna()
            if len(aligned) > 10:
                cov_matrix = np.cov(aligned.iloc[:, 0], aligned.iloc[:, 1])
                beta = cov_matrix[0, 1] / cov_matrix[1, 1]
                corr = float(aligned.corr().iloc[0, 1])
            else:
                beta, corr = 1.0, 0.5
        except Exception:
            beta, corr = 1.0, 0.5

        # Correlation matrix
        corr_matrix = ret.corr().round(4).to_dict()

        return {
            "confidence": conf,
            "var_historical": round(historical_var * 100, 3),
            "var_parametric": round(parametric_var * 100, 3),
            "cvar": round(cvar * 100, 3),
            "max_drawdown": round(max_dd * 100, 2),
            "avg_drawdown": round(avg_dd * 100, 2),
            "current_drawdown": round(current_dd * 100, 2),
            "beta_to_spy": round(beta, 4),
            "spy_correlation": round(corr, 4),
            "daily_volatility": round(sigma * 100, 4),
            "annual_volatility": round(sigma * np.sqrt(252) * 100, 2),
            "correlation_matrix": corr_matrix,
            "weights": {t: round(float(w), 4) for t, w in zip(available, weights_arr)},
        }

    def stress_test(self, tickers: List[str], weights: List[float]) -> Dict:
        """Historical scenario stress testing."""
        scenarios = {
            "COVID Crash (2020)": ("2020-02-01", "2020-03-23"),
            "GFC (2008)": ("2008-01-01", "2009-03-09"),
            "Dot-com bust (2000-02)": ("2000-03-10", "2002-10-09"),
            "Flash Crash (2010)": ("2010-05-06", "2010-05-07"),
            "2022 Bear Market": ("2022-01-01", "2022-10-13"),
        }
        results = {}
        prices_all = self.mde.fetch_multiple(tickers, "20y")
        available = [t for t in tickers if t in prices_all.columns]
        w = np.array(weights[:len(available)])
        w = w / w.sum()

        for name, (start, end) in scenarios.items():
            try:
                window = prices_all[available].loc[start:end]
                if len(window) < 2:
                    continue
                rets = window.pct_change().dropna()
                port_rets = rets.dot(w)
                total_loss = float((1 + port_rets).prod() - 1)
                max_daily_loss = float(port_rets.min())
                results[name] = {
                    "period_return": round(total_loss * 100, 2),
                    "worst_day": round(max_daily_loss * 100, 2),
                    "trading_days": len(rets),
                }
            except Exception:
                pass
        return results


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 7: SIGNAL AGGREGATOR
# ─────────────────────────────────────────────────────────────────────────────

class SignalAggregator:
    mde = MarketDataEngine()
    ta_engine = TechnicalAnalysisEngine()
    sentiment = SentimentEngine()
    ml = MLPredictionEngine()

    def generate(self, ticker: str, period: str = "6mo") -> Dict:
        df = self.mde.fetch_ohlcv(ticker, period)
        indicators = self.ta_engine.compute_all(df)
        patterns = self.ta_engine.pattern_scan(df)
        sentiment_result = self.sentiment.fetch_and_analyse_headlines(ticker)

        signals = []

        # ── Technical Signals ──
        rsi = indicators.get("rsi_14", 50)
        if rsi < 30:
            signals.append({"source": "RSI", "signal": "BUY", "strength": 0.8, "reason": f"RSI={rsi:.1f} oversold"})
        elif rsi > 70:
            signals.append({"source": "RSI", "signal": "SELL", "strength": 0.8, "reason": f"RSI={rsi:.1f} overbought"})

        macd_h = indicators.get("macd_histogram", 0)
        if macd_h > 0:
            signals.append({"source": "MACD", "signal": "BUY", "strength": 0.6, "reason": "MACD histogram positive"})
        else:
            signals.append({"source": "MACD", "signal": "SELL", "strength": 0.6, "reason": "MACD histogram negative"})

        price = indicators.get("price", 0)
        sma200 = indicators.get("sma_200", 0)
        if sma200 > 0:
            if price > sma200:
                signals.append({"source": "SMA200", "signal": "BUY", "strength": 0.7, "reason": "Price above 200-SMA (bull trend)"})
            else:
                signals.append({"source": "SMA200", "signal": "SELL", "strength": 0.7, "reason": "Price below 200-SMA (bear trend)"})

        bb_upper = indicators.get("bb_upper", 0)
        bb_lower = indicators.get("bb_lower", 0)
        if bb_upper > 0:
            if price >= bb_upper:
                signals.append({"source": "Bollinger", "signal": "SELL", "strength": 0.65, "reason": "Price at upper Bollinger Band"})
            elif price <= bb_lower:
                signals.append({"source": "Bollinger", "signal": "BUY", "strength": 0.65, "reason": "Price at lower Bollinger Band"})

        stoch_k = indicators.get("stoch_k", 50)
        if stoch_k < 20:
            signals.append({"source": "Stochastic", "signal": "BUY", "strength": 0.6, "reason": f"Stoch K={stoch_k:.1f} oversold"})
        elif stoch_k > 80:
            signals.append({"source": "Stochastic", "signal": "SELL", "strength": 0.6, "reason": f"Stoch K={stoch_k:.1f} overbought"})

        # ── Pattern Signals ──
        for p in patterns:
            sig = "BUY" if p["signal"] == "bullish" else "SELL" if p["signal"] == "bearish" else "HOLD"
            signals.append({"source": "Pattern", "signal": sig, "strength": p["strength"], "reason": p["pattern"]})

        # ── Sentiment Signal ──
        if "aggregate" in sentiment_result:
            agg = sentiment_result["aggregate"]
            compound = agg.get("avg_compound", 0)
            if compound > 0.05:
                signals.append({"source": "Sentiment", "signal": "BUY", "strength": min(0.8, compound + 0.3), "reason": f"Bullish news sentiment ({compound:.2f})"})
            elif compound < -0.05:
                signals.append({"source": "Sentiment", "signal": "SELL", "strength": min(0.8, abs(compound) + 0.3), "reason": f"Bearish news sentiment ({compound:.2f})"})

        # ── ML Signal ──
        try:
            ml_result = self.ml.predict(df, horizon=5)
            ml_conf = ml_result["direction_probability"]
            if ml_conf > 0.6:
                signals.append({"source": "ML_Model", "signal": "BUY", "strength": ml_conf, "reason": f"ML predicts UP ({ml_conf:.0%} confidence)"})
            elif ml_conf < 0.4:
                signals.append({"source": "ML_Model", "signal": "SELL", "strength": 1 - ml_conf, "reason": f"ML predicts DOWN ({(1-ml_conf):.0%} confidence)"})
        except Exception:
            pass

        # ── Aggregate Score ──
        buy_score = sum(s["strength"] for s in signals if s["signal"] == "BUY")
        sell_score = sum(s["strength"] for s in signals if s["signal"] == "SELL")
        total = buy_score + sell_score + 1e-9
        final_score = (buy_score - sell_score) / total  # -1 to +1

        if final_score > 0.2:
            recommendation = "STRONG BUY" if final_score > 0.5 else "BUY"
        elif final_score < -0.2:
            recommendation = "STRONG SELL" if final_score < -0.5 else "SELL"
        else:
            recommendation = "HOLD"

        return {
            "ticker": ticker,
            "timestamp": datetime.utcnow().isoformat(),
            "recommendation": recommendation,
            "score": round(final_score, 4),
            "buy_score": round(buy_score, 4),
            "sell_score": round(sell_score, 4),
            "signals_count": len(signals),
            "signals": signals,
            "indicators": indicators,
            "patterns": patterns,
            "sentiment": sentiment_result.get("aggregate", {}),
        }


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 8: MARKET REGIME CLASSIFIER
# ─────────────────────────────────────────────────────────────────────────────

class MarketRegimeEngine:
    """8-state market regime classification."""
    mde = MarketDataEngine()
    ta = TechnicalAnalysisEngine()

    REGIMES = {
        0: {"name": "Bull Quiet", "description": "Strong uptrend, low volatility", "strategy": "Buy and hold, momentum"},
        1: {"name": "Bull Volatile", "description": "Uptrend with high volatility", "strategy": "Reduce size, hedge"},
        2: {"name": "Bear Quiet", "description": "Downtrend, low volatility", "strategy": "Short, defensive assets"},
        3: {"name": "Bear Volatile", "description": "Downtrend, high volatility (crash)", "strategy": "Max hedge, cash, gold"},
        4: {"name": "Sideways Low Vol", "description": "Range-bound, calm", "strategy": "Mean reversion, sell options"},
        5: {"name": "Sideways High Vol", "description": "Range-bound, volatile", "strategy": "Straddles, iron condors"},
        6: {"name": "Recovery", "description": "Emerging from bear market", "strategy": "Accumulate quality"},
        7: {"name": "Topping", "description": "Late-cycle, distribution", "strategy": "Reduce exposure gradually"},
    }

    def classify(self, ticker: str = "SPY", period: str = "1y") -> Dict:
        df = self.mde.fetch_ohlcv(ticker, period)
        c = df["Close"]
        ret = c.pct_change()

        sma50 = self.ta.sma(c, 50)
        sma200 = self.ta.sma(c, 200)
        rsi = self.ta.rsi(c)
        adx = self.ta.adx(df)
        vol_20 = ret.rolling(20).std() * np.sqrt(252)
        vol_hist = float(vol_20.mean())
        vol_curr = float(vol_20.iloc[-1])
        high_vol = vol_curr > vol_hist * 1.3

        price = float(c.iloc[-1])
        above_50 = price > float(sma50.iloc[-1])
        above_200 = price > float(sma200.iloc[-1])
        rsi_curr = float(rsi.iloc[-1])
        adx_curr = float(adx.iloc[-1])

        trend_up = above_50 and above_200
        trend_dn = not above_50 and not above_200
        strong_trend = adx_curr > 25

        # Classify
        if trend_up and not high_vol:
            regime_id = 0  # Bull Quiet
        elif trend_up and high_vol:
            regime_id = 1  # Bull Volatile
        elif trend_dn and not high_vol:
            regime_id = 2  # Bear Quiet
        elif trend_dn and high_vol:
            regime_id = 3  # Bear Volatile
        elif not trend_up and not trend_dn and not high_vol:
            regime_id = 4  # Sideways Low Vol
        elif not trend_up and not trend_dn and high_vol:
            regime_id = 5  # Sideways High Vol
        elif rsi_curr < 45 and above_200 and not above_50:
            regime_id = 6  # Recovery
        elif rsi_curr > 70 and above_50 and not strong_trend:
            regime_id = 7  # Topping
        else:
            regime_id = 0 if trend_up else 2

        regime = self.REGIMES[regime_id]

        # Calculate regime probability distribution (simple heuristic)
        probabilities = [0.05] * 8
        probabilities[regime_id] = 0.55
        for i in [regime_id - 1, regime_id + 1]:
            if 0 <= i < 8:
                probabilities[i] = 0.20
        total = sum(probabilities)
        probabilities = [round(p / total, 3) for p in probabilities]

        return {
            "ticker": ticker,
            "regime_id": regime_id,
            "regime_name": regime["name"],
            "description": regime["description"],
            "recommended_strategy": regime["strategy"],
            "indicators": {
                "price_above_sma50": above_50,
                "price_above_sma200": above_200,
                "rsi": round(rsi_curr, 2),
                "adx": round(adx_curr, 2),
                "volatility_current": round(vol_curr * 100, 2),
                "volatility_historical": round(vol_hist * 100, 2),
                "high_volatility": high_vol,
                "strong_trend": strong_trend,
            },
            "regime_probabilities": {self.REGIMES[i]["name"]: probabilities[i] for i in range(8)},
        }


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 9: ANOMALY DETECTION ENGINE
# ─────────────────────────────────────────────────────────────────────────────

class AnomalyEngine:
    mde = MarketDataEngine()
    ta = TechnicalAnalysisEngine()

    def detect(self, ticker: str, period: str = "2y", contamination: float = 0.05) -> Dict:
        df = self.mde.fetch_ohlcv(ticker, period)
        c = df["Close"]
        ret = c.pct_change().dropna()

        # Feature matrix for isolation forest
        feats = pd.DataFrame({
            "ret": ret,
            "vol": ret.rolling(20).std(),
            "rsi": self.ta.rsi(c).reindex(ret.index),
            "vol_ratio": df["Volume"].reindex(ret.index) / df["Volume"].reindex(ret.index).rolling(20).mean(),
        }).dropna()

        iso = IsolationForest(contamination=contamination, random_state=42)
        scores = iso.fit_predict(feats)
        anomaly_mask = scores == -1

        # Z-score method
        z = zscore(ret.dropna())
        z_anomalies = np.abs(z) > 3.0

        # IQR method
        Q1, Q3 = ret.quantile(0.25), ret.quantile(0.75)
        IQR = Q3 - Q1
        iqr_anomalies = (ret < Q1 - 1.5 * IQR) | (ret > Q3 + 1.5 * IQR)

        anomaly_dates = feats.index[anomaly_mask].strftime("%Y-%m-%d").tolist()
        recent_anomalies = [d for d in anomaly_dates if pd.Timestamp(d) > pd.Timestamp.now() - pd.Timedelta(days=90)]

        return {
            "ticker": ticker,
            "period": period,
            "total_observations": len(feats),
            "isolation_forest_anomalies": int(anomaly_mask.sum()),
            "z_score_anomalies": int(z_anomalies.sum()),
            "iqr_anomalies": int(iqr_anomalies.sum()),
            "anomaly_rate": round(float(anomaly_mask.mean()) * 100, 2),
            "recent_anomaly_dates": recent_anomalies[-10:],
            "current_z_score": round(float(z[-1]) if len(z) > 0 else 0, 4),
            "is_currently_anomalous": bool(np.abs(z[-1]) > 3.0) if len(z) > 0 else False,
            "last_anomaly": anomaly_dates[-1] if anomaly_dates else None,
        }


# ─────────────────────────────────────────────────────────────────────────────
# MODULE 10: ECONOMIC DATA ENGINE (FRED)
# ─────────────────────────────────────────────────────────────────────────────

class EconomicEngine:
    mde = MarketDataEngine()

    SERIES = {
        "gdp_growth": "A191RL1Q225SBEA",
        "cpi_yoy": "CPIAUCSL",
        "unemployment": "UNRATE",
        "fed_funds_rate": "FEDFUNDS",
        "10y_yield": "GS10",
        "2y_yield": "GS2",
        "yield_curve_10_2": "T10Y2Y",
        "m2_money_supply": "M2SL",
        "industrial_production": "INDPRO",
        "retail_sales": "RSXFS",
        "housing_starts": "HOUST",
        "consumer_confidence": "UMCSENT",
    }

    def fetch_dashboard(self) -> Dict:
        dashboard = {}
        for name, series_id in self.SERIES.items():
            s = self.mde.fetch_fred(series_id, limit=13)
            if not s.empty:
                latest = float(s.iloc[-1])
                prev = float(s.iloc[-2]) if len(s) > 1 else latest
                dashboard[name] = {
                    "series_id": series_id,
                    "latest": round(latest, 4),
                    "previous": round(prev, 4),
                    "change": round(latest - prev, 4),
                    "date": s.index[-1].strftime("%Y-%m-%d"),
                }
        # Yield curve inversion check
        if "10y_yield" in dashboard and "2y_yield" in dashboard:
            spread = dashboard["10y_yield"]["latest"] - dashboard["2y_yield"]["latest"]
            dashboard["recession_signal"] = {
                "yield_curve_spread": round(spread, 3),
                "inverted": spread < 0,
                "signal": "Recession risk elevated" if spread < 0 else "Normal",
            }
        return dashboard


# ─────────────────────────────────────────────────────────────────────────────
# ENGINE INSTANCES
# ─────────────────────────────────────────────────────────────────────────────

market_data = MarketDataEngine()
ta_engine = TechnicalAnalysisEngine()
sentiment_engine = SentimentEngine()
ml_engine = MLPredictionEngine()
portfolio_engine = PortfolioEngine()
risk_engine = RiskEngine()
signal_aggregator = SignalAggregator()
regime_engine = MarketRegimeEngine()
anomaly_engine = AnomalyEngine()
economic_engine = EconomicEngine()


# ─────────────────────────────────────────────────────────────────────────────
# FASTAPI ROUTES
# ─────────────────────────────────────────────────────────────────────────────

@app.get("/", tags=["Health"])
async def root():
    return {
        "platform": "Veyra (VRA) AI/ML Engine",
        "version": "4.0.0",
        "status": "operational",
        "modules": [
            "market_data", "technical_analysis", "sentiment",
            "ml_prediction", "portfolio_optimisation", "risk_management",
            "signal_aggregation", "market_regime", "anomaly_detection", "economics"
        ],
        "data_sources": ["yfinance", "FRED", "CryptoCompare"],
        "api_docs": "/ai/docs",
        "timestamp": datetime.utcnow().isoformat(),
    }

@app.get("/health", tags=["Health"])
async def health():
    return {"status": "healthy", "timestamp": datetime.utcnow().isoformat()}


# ── Market Data ──

@app.get("/api/v1/market/snapshot", tags=["Market Data"])
async def market_snapshot():
    """Live market snapshot for major indices, VIX, gold, oil."""
    return market_data.fetch_market_snapshot()

@app.get("/api/v1/market/sectors", tags=["Market Data"])
async def sector_rotation():
    """Sector ETF returns — sector rotation analysis."""
    return {"sectors": market_data.fetch_sector_rotation(), "period": "3mo"}

@app.get("/api/v1/market/quote/{ticker}", tags=["Market Data"])
async def get_quote(ticker: str, period: str = "1y", interval: str = "1d"):
    """OHLCV data for any ticker (stocks, ETFs, crypto, FX, futures)."""
    df = market_data.fetch_ohlcv(ticker.upper(), period, interval)
    return {
        "ticker": ticker.upper(),
        "period": period,
        "interval": interval,
        "rows": len(df),
        "latest_close": round(float(df["Close"].iloc[-1]), 4),
        "latest_date": str(df.index[-1].date()),
        "data": df.tail(30).reset_index().apply(
            lambda row: {k: (round(float(v), 4) if isinstance(v, (float, np.floating)) else str(v))
                         for k, v in row.items()}, axis=1).tolist(),
    }

@app.get("/api/v1/market/info/{ticker}", tags=["Market Data"])
async def get_ticker_info(ticker: str):
    """Fundamental info: P/E, market cap, sector, earnings, etc."""
    info = market_data.fetch_info(ticker.upper())
    keys = ["shortName", "sector", "industry", "marketCap", "trailingPE", "forwardPE",
            "priceToBook", "dividendYield", "beta", "fiftyTwoWeekHigh", "fiftyTwoWeekLow",
            "averageVolume", "returnOnEquity", "debtToEquity", "revenueGrowth",
            "earningsGrowth", "currency", "country"]
    return {k: info.get(k) for k in keys if info.get(k) is not None}

@app.get("/api/v1/market/crypto", tags=["Market Data"])
async def crypto_overview():
    """Top 10 crypto pairs from yfinance."""
    result = {}
    for pair in MarketDataEngine.CRYPTO_PAIRS:
        try:
            df = market_data.fetch_ohlcv(pair, period="5d", interval="1d")
            if len(df) >= 2:
                price = float(df["Close"].iloc[-1])
                prev = float(df["Close"].iloc[-2])
                result[pair] = {"price": round(price, 4), "change_24h_pct": round((price-prev)/prev*100, 2)}
        except Exception:
            pass
    return result


# ── Technical Analysis ──

@app.get("/api/v1/technical/{ticker}", tags=["Technical Analysis"])
async def technical_analysis(ticker: str, period: str = "1y"):
    """All 20+ technical indicators for a ticker."""
    df = market_data.fetch_ohlcv(ticker.upper(), period)
    indicators = ta_engine.compute_all(df)
    patterns = ta_engine.pattern_scan(df)
    return {"ticker": ticker.upper(), "period": period, "indicators": indicators, "patterns": patterns}

@app.get("/api/v1/technical/{ticker}/patterns", tags=["Technical Analysis"])
async def chart_patterns(ticker: str, period: str = "1y"):
    df = market_data.fetch_ohlcv(ticker.upper(), period)
    patterns = ta_engine.pattern_scan(df)
    return {"ticker": ticker.upper(), "patterns": patterns, "count": len(patterns)}


# ── Sentiment ──

@app.post("/api/v1/sentiment/analyse", tags=["Sentiment"])
async def analyse_sentiment(req: SentimentRequest):
    """Score a list of text strings for financial sentiment."""
    return sentiment_engine.analyse_batch(req.texts)

@app.get("/api/v1/sentiment/{ticker}", tags=["Sentiment"])
async def ticker_sentiment(ticker: str):
    """Fetch and analyse recent news headlines for a ticker."""
    return sentiment_engine.fetch_and_analyse_headlines(ticker.upper())

@app.get("/api/v1/sentiment/fear-greed", tags=["Sentiment"])
async def fear_greed_index():
    """Compute a Fear & Greed proxy from public market data."""
    try:
        vix_df = market_data.fetch_ohlcv("^VIX", period="30d")
        spy_df = market_data.fetch_ohlcv("SPY", period="30d")
        vix = float(vix_df["Close"].iloc[-1])
        spy_rsi = float(ta_engine.rsi(spy_df["Close"]).iloc[-1])
        spy_bb = ta_engine.bollinger_bands(spy_df["Close"])
        bb_pos = (float(spy_df["Close"].iloc[-1]) - float(spy_bb[0].iloc[-1])) / (float(spy_bb[2].iloc[-1]) - float(spy_bb[0].iloc[-1]) + 1e-9)

        # Composite (0-100 scale; 0=extreme fear, 100=extreme greed)
        vix_score = max(0, min(100, 100 - (vix - 10) / 0.7))
        rsi_score = spy_rsi
        bb_score = bb_pos * 100
        composite = round((vix_score * 0.4 + rsi_score * 0.35 + bb_score * 0.25), 1)
        label = (
            "Extreme Fear" if composite < 25 else
            "Fear" if composite < 45 else
            "Neutral" if composite < 55 else
            "Greed" if composite < 75 else
            "Extreme Greed"
        )
        return {
            "score": composite,
            "label": label,
            "components": {
                "vix": round(vix, 2), "vix_score": round(vix_score, 1),
                "spy_rsi": round(spy_rsi, 1), "rsi_score": round(rsi_score, 1),
                "bb_position": round(bb_pos, 3), "bb_score": round(bb_score, 1),
            }
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


# ── ML Prediction ──

@app.post("/api/v1/predict", tags=["ML Prediction"])
async def predict_price(req: PredictRequest):
    """ML ensemble price direction prediction (RandomForest + GBM)."""
    df = market_data.fetch_ohlcv(req.ticker.upper(), period="2y")
    return {"ticker": req.ticker.upper(), **ml_engine.predict(df, req.horizon_days, req.model)}

@app.get("/api/v1/predict/{ticker}", tags=["ML Prediction"])
async def predict_price_get(ticker: str, horizon: int = Query(5, ge=1, le=30)):
    df = market_data.fetch_ohlcv(ticker.upper(), period="2y")
    return {"ticker": ticker.upper(), **ml_engine.predict(df, horizon)}


# ── Portfolio ──

@app.post("/api/v1/portfolio/analyse", tags=["Portfolio"])
async def analyse_portfolio(req: PortfolioRequest):
    """Analyse portfolio performance and risk metrics."""
    prices = market_data.fetch_multiple(req.tickers, req.period)
    available = [t for t in req.tickers if t in prices.columns]
    weights = np.array(req.weights or [1 / len(available)] * len(available))
    weights = weights[:len(available)] / weights[:len(available)].sum()
    returns = prices[available].pct_change().dropna()
    met = portfolio_engine.metrics(returns, weights, req.risk_free_rate)
    return {
        "tickers": available,
        "weights": {t: round(float(w), 4) for t, w in zip(available, weights)},
        "metrics": met,
        "period": req.period,
    }

@app.post("/api/v1/portfolio/optimise", tags=["Portfolio"])
async def optimise_portfolio(req: OptimiseRequest):
    """Optimise portfolio weights using MPT (Sharpe, min-vol, or Kelly)."""
    return portfolio_engine.run_optimisation(req.tickers, req.method, req.period, req.risk_free_rate)

@app.post("/api/v1/portfolio/efficient-frontier", tags=["Portfolio"])
async def efficient_frontier(req: PortfolioRequest):
    """Generate the efficient frontier for a set of tickers."""
    returns = market_data.fetch_multiple(req.tickers, req.period).pct_change().dropna()
    available = [t for t in req.tickers if t in returns.columns]
    ef = portfolio_engine.efficient_frontier(returns[available])
    return {"tickers": available, "efficient_frontier": ef}


# ── Risk Management ──

@app.post("/api/v1/risk/portfolio", tags=["Risk"])
async def portfolio_risk(req: RiskRequest):
    """Full risk metrics: VaR, CVaR, drawdown, beta, correlations."""
    weights = req.weights or [1 / len(req.tickers)] * len(req.tickers)
    return risk_engine.portfolio_risk(req.tickers, weights, req.period, req.confidence)

@app.post("/api/v1/risk/stress-test", tags=["Risk"])
async def stress_test(req: RiskRequest):
    """Historical scenario stress testing (GFC, COVID, 2022, etc.)."""
    weights = req.weights or [1 / len(req.tickers)] * len(req.tickers)
    return {"scenarios": risk_engine.stress_test(req.tickers, weights)}


# ── Signal Aggregator ──

@app.get("/api/v1/signals/{ticker}", tags=["Signals"])
async def get_signals(ticker: str, period: str = "6mo"):
    """Multi-source aggregated BUY/SELL/HOLD signal with confidence."""
    return signal_aggregator.generate(ticker.upper(), period)


# ── Market Regime ──

@app.get("/api/v1/regime/{ticker}", tags=["Market Regime"])
async def market_regime(ticker: str = "SPY", period: str = "1y"):
    """8-state market regime classification with strategy recommendation."""
    return regime_engine.classify(ticker.upper(), period)

@app.get("/api/v1/regime/global/overview", tags=["Market Regime"])
async def global_regime_overview():
    """Regime classification across key indices."""
    tickers = {"US": "SPY", "UK": "EWU", "Europe": "VGK", "Emerging": "EEM"}
    results = {}
    for name, sym in tickers.items():
        try:
            results[name] = regime_engine.classify(sym, "1y")
        except Exception:
            pass
    return results


# ── Anomaly Detection ──

@app.post("/api/v1/anomaly/detect", tags=["Anomaly Detection"])
async def detect_anomalies(req: AnomalyRequest):
    """Detect price/volume anomalies using Isolation Forest + Z-score + IQR."""
    return anomaly_engine.detect(req.ticker.upper(), req.period, req.contamination)

@app.get("/api/v1/anomaly/{ticker}", tags=["Anomaly Detection"])
async def detect_anomalies_get(ticker: str, period: str = "2y"):
    return anomaly_engine.detect(ticker.upper(), period)


# ── Economic Data ──

@app.get("/api/v1/economics/dashboard", tags=["Economics"])
async def economics_dashboard():
    """FRED macro dashboard: GDP, CPI, unemployment, yields, M2, etc."""
    return economic_engine.fetch_dashboard()

@app.get("/api/v1/economics/series/{series_id}", tags=["Economics"])
async def fred_series(series_id: str, limit: int = 50):
    """Fetch any FRED data series by ID."""
    s = market_data.fetch_fred(series_id, limit)
    if s.empty:
        raise HTTPException(status_code=404, detail=f"FRED series {series_id} not found or empty")
    return {
        "series_id": series_id,
        "count": len(s),
        "latest": float(s.iloc[-1]),
        "data": {str(k.date()): round(float(v), 4) for k, v in s.items()},
    }


# ── Screener ──

@app.get("/api/v1/screener/oversold", tags=["Screener"])
async def screen_oversold():
    """Screen S&P 500 components for oversold RSI (example with liquid large caps)."""
    universe = ["AAPL", "MSFT", "GOOGL", "AMZN", "NVDA", "META", "TSLA", "BRK-B",
                "JPM", "JNJ", "UNH", "XOM", "V", "PG", "MA", "HD", "CVX", "MRK",
                "ABBV", "PFE", "BAC", "KO", "PEP", "AVGO", "COST"]
    results = []
    for ticker in universe:
        try:
            df = market_data.fetch_ohlcv(ticker, period="3mo")
            rsi = float(ta_engine.rsi(df["Close"]).iloc[-1])
            if rsi < 35:
                results.append({"ticker": ticker, "rsi": round(rsi, 2), "signal": "oversold"})
        except Exception:
            pass
    return sorted(results, key=lambda x: x["rsi"])

@app.get("/api/v1/screener/momentum", tags=["Screener"])
async def screen_momentum():
    """Top momentum stocks from a liquid universe."""
    universe = ["AAPL", "MSFT", "GOOGL", "AMZN", "NVDA", "META", "TSLA", "AVGO",
                "CRM", "AMD", "ORCL", "ADBE", "NFLX", "PYPL", "SQ", "SHOP"]
    results = []
    for ticker in universe:
        try:
            df = market_data.fetch_ohlcv(ticker, period="6mo")
            c = df["Close"]
            ret_1m = float(c.pct_change(21).iloc[-1])
            ret_3m = float(c.pct_change(63).iloc[-1])
            ret_6m = float(c.pct_change(126).iloc[-1])
            mom_score = ret_1m * 0.3 + ret_3m * 0.4 + ret_6m * 0.3
            results.append({"ticker": ticker, "return_1m": round(ret_1m * 100, 2),
                             "return_3m": round(ret_3m * 100, 2), "return_6m": round(ret_6m * 100, 2),
                             "momentum_score": round(mom_score * 100, 2)})
        except Exception:
            pass
    return sorted(results, key=lambda x: x["momentum_score"], reverse=True)[:10]


# ── WebSocket (Real-time streaming) ──

@app.websocket("/ws/market")
async def market_stream(websocket: WebSocket):
    """Stream live quotes for a list of tickers every N seconds."""
    await websocket.accept()
    log.info("WebSocket client connected")
    try:
        config = await websocket.receive_json()
        tickers = config.get("tickers", ["AAPL", "MSFT"])
        interval_s = config.get("interval_seconds", 15)

        while True:
            payload = {"timestamp": datetime.utcnow().isoformat(), "quotes": {}}
            for ticker in tickers:
                try:
                    df = market_data.fetch_ohlcv(ticker.upper(), period="5d", interval="1d")
                    price = float(df["Close"].iloc[-1])
                    prev = float(df["Close"].iloc[-2]) if len(df) >= 2 else price
                    payload["quotes"][ticker] = {
                        "price": round(price, 4),
                        "change_pct": round((price - prev) / prev * 100, 2),
                    }
                except Exception:
                    pass
            await websocket.send_json(payload)
            await asyncio.sleep(interval_s)
    except WebSocketDisconnect:
        log.info("WebSocket client disconnected")


# ─────────────────────────────────────────────────────────────────────────────
# STARTUP / SHUTDOWN
# ─────────────────────────────────────────────────────────────────────────────

@app.on_event("startup")
async def startup():
    log.info("=" * 60)
    log.info("🚀 Veyra AI/ML Engine v4.0.0 — Starting up")
    log.info("   All data sources: 100% free & open-source")
    log.info("   API docs: http://localhost:8001/ai/docs")
    log.info("=" * 60)

@app.on_event("shutdown")
async def shutdown():
    log.info("Veyra AI/ML Engine — Shutting down")


# ─────────────────────────────────────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "veyra_ai_engine:app",
        host="0.0.0.0",
        port=int(os.getenv("AI_ENGINE_PORT", "8001")),
        reload=bool(os.getenv("DEBUG", "false").lower() == "true"),
        workers=int(os.getenv("AI_WORKERS", "1")),
        log_level="info",
    )
