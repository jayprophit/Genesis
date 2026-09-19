# Veyra AI/ML Engine — Integration Guide

**File:** `src/backend/ai/veyra_ai_engine.py`  
**Port:** `8001` (runs alongside main FastAPI app on `8000`)  
**Data Sources:** 100% free — yfinance, FRED, yfinance crypto  
**API Keys Required:** None

---

## Drop-in Location

```
financial-master/
└── src/
    └── backend/
        └── ai/
            ├── veyra_ai_engine.py   ← main engine (this file)
            └── __init__.py
```

---

## Quick Start

```bash
# Install dependencies
pip install -r requirements_ai.txt

# Run the engine
uvicorn veyra_ai_engine:app --host 0.0.0.0 --port 8001 --reload

# Or with Python directly
python veyra_ai_engine.py

# API Docs (Swagger UI)
open http://localhost:8001/ai/docs
```

---

## Docker (add to docker-compose.yml)

```yaml
ai-engine:
  build: .
  command: uvicorn src.backend.ai.veyra_ai_engine:app --host 0.0.0.0 --port 8001
  ports:
    - "8001:8001"
  environment:
    - AI_ENGINE_PORT=8001
    - AI_WORKERS=2
    - DEBUG=false
    - FRED_API_KEY=${FRED_API_KEY}   # optional — many FRED series work without it
  volumes:
    - ./src:/app/src
  depends_on:
    - redis
  restart: unless-stopped
```

---

## API Endpoints (40+)

### Health
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Engine info & module list |
| GET | `/health` | Health check |

### Market Data
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/v1/market/snapshot` | Live indices, VIX, gold, oil |
| GET | `/api/v1/market/sectors` | Sector rotation (3-month returns) |
| GET | `/api/v1/market/quote/{ticker}` | OHLCV for any ticker |
| GET | `/api/v1/market/info/{ticker}` | Fundamentals (P/E, market cap, etc.) |
| GET | `/api/v1/market/crypto` | Top 10 crypto pairs |

### Technical Analysis
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/v1/technical/{ticker}` | All 20+ indicators |
| GET | `/api/v1/technical/{ticker}/patterns` | Candlestick & chart patterns |

### Sentiment
| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/v1/sentiment/analyse` | Score custom text list |
| GET | `/api/v1/sentiment/{ticker}` | News headline sentiment |
| GET | `/api/v1/sentiment/fear-greed` | Fear & Greed proxy index |

### ML Prediction
| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/v1/predict` | Price direction prediction |
| GET | `/api/v1/predict/{ticker}?horizon=5` | Quick prediction GET |

### Portfolio
| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/v1/portfolio/analyse` | Sharpe, Sortino, VaR, drawdown |
| POST | `/api/v1/portfolio/optimise` | MPT optimisation (Sharpe/min-vol/Kelly) |
| POST | `/api/v1/portfolio/efficient-frontier` | Efficient frontier curve |

### Risk
| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/v1/risk/portfolio` | VaR, CVaR, beta, correlations |
| POST | `/api/v1/risk/stress-test` | Historical scenarios (GFC, COVID, etc.) |

### Signals
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/v1/signals/{ticker}` | Aggregated BUY/SELL/HOLD signal |

### Market Regime
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/v1/regime/{ticker}` | 8-state regime + strategy |
| GET | `/api/v1/regime/global/overview` | Global regime overview |

### Anomaly Detection
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/v1/anomaly/{ticker}` | Price/volume anomalies |
| POST | `/api/v1/anomaly/detect` | Custom contamination rate |

### Economics (FRED)
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/v1/economics/dashboard` | GDP, CPI, unemployment, yields, M2 |
| GET | `/api/v1/economics/series/{id}` | Any FRED series |

### Screener
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/v1/screener/oversold` | RSI < 35 scan |
| GET | `/api/v1/screener/momentum` | Top momentum stocks |

### WebSocket
| Protocol | Endpoint | Description |
|----------|----------|-------------|
| WS | `/ws/market` | Real-time quote streaming |

**WebSocket usage:**
```javascript
const ws = new WebSocket("ws://localhost:8001/ws/market");
ws.onopen = () => ws.send(JSON.stringify({
  tickers: ["AAPL", "MSFT", "NVDA"],
  interval_seconds: 15
}));
ws.onmessage = (e) => console.log(JSON.parse(e.data));
```

---

## Modules Breakdown

| Module | Class | Free Source |
|--------|-------|-------------|
| Market Data | `MarketDataEngine` | yfinance, FRED |
| Technical Analysis | `TechnicalAnalysisEngine` | Pure numpy (no TA-Lib) |
| Sentiment | `SentimentEngine` | VADER (offline) |
| ML Prediction | `MLPredictionEngine` | scikit-learn |
| Portfolio Optimisation | `PortfolioEngine` | scipy.optimize |
| Risk Management | `RiskEngine` | numpy/pandas |
| Signal Aggregation | `SignalAggregator` | All above combined |
| Market Regime | `MarketRegimeEngine` | Rule-based HMM-lite |
| Anomaly Detection | `AnomalyEngine` | sklearn IsolationForest |
| Economic Data | `EconomicEngine` | FRED public API |

---

## Environment Variables (.env)

```env
# AI Engine
AI_ENGINE_PORT=8001
AI_WORKERS=2
DEBUG=false

# Optional: FRED API key (increases rate limits)
# Free key at: https://fred.stlouisfed.org/docs/api/api_key.html
FRED_API_KEY=your_key_here

# Cache duration in seconds (default: 300)
CACHE_DURATION=300
```

---

## Upgrade Path (Hugging Face)

To swap VADER for FinBERT (GPU recommended):

```python
# In SentimentEngine.analyse(), replace vader.polarity_scores() with:
from transformers import pipeline
finbert = pipeline("sentiment-analysis", model="ProsusAI/finbert")
result = finbert(text)[0]
```

No other changes needed — same API contract.

---

## Change History

| Date | Version | Change |
|------|---------|--------|
| 2026-05-10 | v4.0.0 | Initial release — 10 modules, 40+ endpoints |
