"""Genesis original models and training tools.

Training uses PyTorch.  NumPy-only native inference does not import PyTorch,
including on `python -m genesis_seed.cli_native` startup.
"""
__all__ = ['GenesisSeed','SeedConfig']

def __getattr__(name: str):
    if name in __all__:
        from .model import GenesisSeed, SeedConfig
        return {'GenesisSeed':GenesisSeed,'SeedConfig':SeedConfig}[name]
    raise AttributeError(name)
