
import sys
sys.path.insert(0, "C:/Users/jpowe/Desktop/Projects/Genesis/experiments/genesis_native_seed")

from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig
print("Import OK")

config = GenesisConfig(seed_config=SeedConfig(width=128, heads=4, layers=2, context=128, dropout=0.0))
model = GenesisSeed(config)
total_params = sum(p.numel() for p in model.parameters())
print("Total parameters:", total_params)
