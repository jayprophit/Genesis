"""Genesis: an independently trained multimodal agent model.

Genesis is not a wrapper around another model. It starts from random weights,
trains on our own data, and carries its own tokenizer, vision tower, tool
protocol and agent loop.
"""
from .config import PRESETS, ModelConfig, VisionConfig
from .model import GenesisModel, KVCache
from .tokenizer import Tokenizer
from .checkpoint import load_checkpoint, save_checkpoint
from .agent import GenesisAgent, GenesisModelBackend, ScriptedBackend
from .tools import ToolRegistry, ToolSpec, ToolContext, default_registry, parse_tool_calls
from .train import TrainConfig, train

__version__ = "0.1.0"

__all__ = [
    "PRESETS", "ModelConfig", "VisionConfig",
    "GenesisModel", "KVCache", "Tokenizer",
    "load_checkpoint", "save_checkpoint",
    "GenesisAgent", "GenesisModelBackend", "ScriptedBackend",
    "ToolRegistry", "ToolSpec", "ToolContext", "default_registry", "parse_tool_calls",
    "TrainConfig", "train",
]
